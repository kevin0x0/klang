#include "include/klapi.h"
#include "include/common/klconfig.h"
#include "include/value/klcoroutine.h"
#include "include/vm/klexception.h"
#include "include/vm/klexec.h"
#include "deps/k/include/kio/ki.h"
#include "deps/k/include/kio/kibuf.h"
#include "deps/k/include/kio/kifile.h"
#include "deps/k/include/kio/kofile.h"
#include "deps/k/include/os_spec/kfs.h"
#include "deps/k/include/string/kstring.h"
#include <stdarg.h>

#define OPTION_NORMAL    ((unsigned)0)
#define OPTION_HELP      ((unsigned)klbit(0))
#define OPTION_INTER     ((unsigned)klbit(1))
#define OPTION_IN_FILE   ((unsigned)klbit(2))
#define OPTION_IN_ARG    ((unsigned)klbit(3))
#define OPTION_UNDUMP    ((unsigned)klbit(4))


#define NARGRAW(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, ...)     (_9)
#define NARG(...)        NARGRAW(__VA_ARGS__ , 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)
#define match(opt, ...)  matchraw((opt), NARG(__VA_ARGS__), __VA_ARGS__)
#define isfilename(arg)  ((arg)[0] != '-')

typedef struct tagBehaviour {
  union {
    char* filename;
    const char* stmt;
  };
  char* corelibpath;
  char** args;
  size_t narg;
  unsigned option;
} Behaviour;

typedef struct tagBasicTool {
  KlCFunction* bcloader;
  KlCFunction* compiler;
  KlCFunction* compileri;
  KlCFunction* traceback;
} BasicTool;

typedef struct tagKiInteractive {
  Ki base;
  unsigned char* buf;
  unsigned char* curr;
  unsigned char* end;
  bool eof;
} KiInteractive;

static void interactive_delete(KiInteractive* ki);
static void interactive_reader(KiInteractive* ki);
static Ki* interactive_create(void);

static KiVirtualFunc interactive_vfunc = {
  .size = NULL,
  .delete = (KiDelete)interactive_delete,
  .reader = (KiReader)interactive_reader,
};

static int parse_argv(int argc, char** argv, Behaviour* behaviour);
static bool matchraw(const char* arg, unsigned nopts, ...);
static void cleanbehaviour(Behaviour* behaviour);
static int validatebehaviour(Behaviour* behaviour);
static void print_help(void);

static int fatalerror(const char* stage, const char* fmt, ...);

static int real_main(Behaviour* behaviour);
static KlException preload(Behaviour* behaviour, KlState* state, BasicTool* btool);
static KlException call_bcloader(KlState* state, Ki* input, Ko* err, KlCFunction* bcloader);
static KlException call_compiler(KlState* state, Ki* input, Ko* err, const char* inputname, const char* srcfile, KlCFunction* compiler);
static KlException execute_script(Behaviour* behaviour, KlState* state, BasicTool* btool, Ko* err);
static KlException interactive(KlState* state, BasicTool* btool, Ko* err);

static KlException errhandler(KlState* state);

int main(int argc, char** argv) {
  Behaviour behaviour;
  int ret = parse_argv(argc, argv, &behaviour);
  if (kl_unlikely(ret))
    return ret;
  return real_main(&behaviour);
}

static int parse_argv(int argc, char** argv, Behaviour* behaviour) {
  behaviour->filename = NULL;
  behaviour->corelibpath = NULL;
  behaviour->args = NULL;
  behaviour->narg = 0;
  behaviour->stmt = NULL;
  behaviour->option = OPTION_NORMAL;
  for (int i = 1; i < argc; ++i) {
    if (match(argv[i], "-h", "--help")) {
      behaviour->option |= OPTION_HELP;
    } else if (match(argv[i], "-e")) {
      if (behaviour->option & (OPTION_IN_FILE | OPTION_IN_ARG)) {
        fprintf(stderr, "the input is specified more than once: %s.\n", argv[i]);
        cleanbehaviour(behaviour);
        return 1;
      }
      if (!argv[++i]) {
        fprintf(stderr, "expected to be executed statement after %s.\n", argv[i - 1]);
        cleanbehaviour(behaviour);
        return 1;
      }
      behaviour->option |= OPTION_IN_ARG;
      behaviour->stmt = argv[i++];
      behaviour->args = argv + i;
      behaviour->narg = argc - i;
      break;
    } else if (match(argv[i], "-i", "--interactive")) {
      behaviour->option |= OPTION_INTER;
    } else if (match(argv[i], "-u", "--undump")) {
      if (++i == argc) {
        fprintf(stderr, "expected <filename> after option: %s\n", argv[i - 1]);
        cleanbehaviour(behaviour);
        return 1;
      }
      if (behaviour->option & (OPTION_IN_FILE | OPTION_IN_ARG)) {
        fprintf(stderr, "the input is specified more than once: %s.\n", argv[i]);
        cleanbehaviour(behaviour);
        return 1;
      }
      behaviour->option |= OPTION_IN_FILE | OPTION_UNDUMP;
      if (!(behaviour->filename = kfs_abspath(argv[i]))) {
        fprintf(stderr, "out of memory while parsing command line\n");
        cleanbehaviour(behaviour);
        return 1;
      }
      behaviour->args = argv + ++i;
      behaviour->narg = argc - i;
      break;
    } else if (match(argv[i], "--corelibpath")) {
      if (++i == argc) {
        fprintf(stderr, "expected <directory path> after option: %s\n", argv[i - 1]);
        cleanbehaviour(behaviour);
        return 1;
      }
      if (behaviour->corelibpath) {
        fprintf(stderr, "the core library path is specified more than once: %s.\n", argv[i]);
        cleanbehaviour(behaviour);
        return 1;
      }
      if (!(behaviour->corelibpath = kstr_copy(argv[i]))) {
        fprintf(stderr, "out of memory while parsing command line\n");
        cleanbehaviour(behaviour);
        return 1;
      }
      size_t len = strlen(behaviour->corelibpath);
      if (len != 0 && (behaviour->corelibpath[len - 1] == '/'
                   || behaviour->corelibpath[len - 1] == '\\')) {
        behaviour->corelibpath[len - 1] = '\0';
      }
    } else if (isfilename(argv[i])) {
      if (behaviour->option & (OPTION_IN_FILE | OPTION_IN_ARG)) {
        fprintf(stderr, "the input is specified more than once: %s.\n", argv[i]);
        cleanbehaviour(behaviour);
        return 1;
      }
      behaviour->option |= OPTION_IN_FILE;
      if (!(behaviour->filename = kfs_abspath(argv[i]))) {
        fprintf(stderr, "out of memory while parsing command line\n");
        cleanbehaviour(behaviour);
        return 1;
      }
      behaviour->args = argv + ++i;
      behaviour->narg = argc - i;
      break;
    } else {
      fprintf(stderr, "unrecognized option: %s\n", argv[i]);
      print_help();
      cleanbehaviour(behaviour);
      return 1;
    }
  }
  return validatebehaviour(behaviour);
}

static int real_main(Behaviour* behaviour) {
  if (behaviour->option & OPTION_HELP) {
    print_help();
    cleanbehaviour(behaviour);
    return 0;
  }
  int failure = 0;
  KlMM klmm;
  klmm_init(&klmm, 32 * 1024);
  KlState* state = klapi_new_state(&klmm);
  if (kl_unlikely(!state)) {
    failure = fatalerror("initializing", "can not create virtual machine");
    goto error_create_vm;
  }
  BasicTool btool = { NULL, NULL, NULL, NULL };
  failure = preload(behaviour, state, &btool);
  if (kl_unlikely(failure)) {
    fprintf(stderr, "%s\n", klapi_exception_message(state));
    goto error_preload;
  }
  Ko* err = kofile_attach(stderr);
  if (kl_unlikely(!err)) {
    failure = fatalerror("initializing", "can not create error output stream");
    goto error_create_err;
  }
  if (behaviour->option & (OPTION_IN_ARG | OPTION_IN_FILE)) {
    failure = execute_script(behaviour, state, &btool, err);
    if (kl_unlikely(failure)) {
      fprintf(stderr, "%s\n", klapi_exception_message(state));
      goto error_execute_script;
    }
  }
  if (behaviour->option & OPTION_INTER) {
    failure = interactive(state, &btool, err);
    if (kl_unlikely(failure))
      fprintf(stderr, "%s\n", klapi_exception_message(state));
  }

error_execute_script:
  ko_delete(err);
error_create_err:
error_preload:
  klmm_destroy(&klmm);
error_create_vm:
  cleanbehaviour(behaviour);
  return failure;
}

static KlException kl_dopreload_helper_loadlib(KlState* state, const char* libname, const char* entryfunc) {
  KLAPI_PROTECT(klapi_checkstack(state, 1));
  KLAPI_PROTECT(klapi_pushstring(state, libname));
  KLAPI_PROTECT(klapi_concati(state, -1, -2, -1));
  KLAPI_PROTECT(klapi_loadlib(state, 0, entryfunc));
  return KL_E_NONE;
}

static KlException preload(Behaviour* behaviour, KlState* state, BasicTool* btool) {
  kl_unused(behaviour);
  KLAPI_PROTECT(klapi_checkstack(state, 1));
  KLAPI_PROTECT(klapi_pushstring(state, behaviour->corelibpath));

  /* load compiler */
  KLAPI_PROTECT(kl_dopreload_helper_loadlib(state, "/runtime_compiler.so", KLCONFIG_LIBRARY_RTCPL_ENTRYFUNCNAME_QUOTE));
  klapi_pop(state, 1); /* pop evaluate, not used here */
  btool->compiler = klapi_getcfunc(state, -3);
  btool->compileri = klapi_getcfunc(state, -2);
  btool->bcloader = klapi_getcfunc(state, -1);
  klapi_pop(state, 3); /* pop 3 results */

  /* load traceback */
  KLAPI_PROTECT(kl_dopreload_helper_loadlib(state, "/traceback.so", KLCONFIG_LIBRARY_TRACEBACK_ENTRYFUNCNAME_QUOTE));
  btool->traceback = klapi_getcfunc(state, -1);
  klapi_pop(state, 1); /* pop 1 result */

  /* load basic */
  KLAPI_PROTECT(kl_dopreload_helper_loadlib(state, "/basic.so", KLCONFIG_LIBRARY_BASIC_ENTRYFUNCNAME_QUOTE));

  /* load stream */
  KLAPI_PROTECT(kl_dopreload_helper_loadlib(state, "/stream.so", KLCONFIG_LIBRARY_STREAM_ENTRYFUNCNAME_QUOTE));

  /* load rtcpl_wrapper */
  KLAPI_PROTECT(kl_dopreload_helper_loadlib(state, "/rtcpl_wrapper.so", KLCONFIG_LIBRARY_RTCPL_WRAPPER_ENTRYFUNCNAME_QUOTE));

  /* load print */
  KLAPI_PROTECT(kl_dopreload_helper_loadlib(state, "/print.so", KLCONFIG_LIBRARY_PRINT_ENTRYFUNCNAME_QUOTE));

  /* load string */
  KLAPI_PROTECT(kl_dopreload_helper_loadlib(state, "/string.so", KLCONFIG_LIBRARY_STRING_ENTRYFUNCNAME_QUOTE));

  /* load cast */
  KLAPI_PROTECT(kl_dopreload_helper_loadlib(state, "/cast.so", KLCONFIG_LIBRARY_CAST_ENTRYFUNCNAME_QUOTE));

  /* load os */
  KLAPI_PROTECT(kl_dopreload_helper_loadlib(state, "/os.so", KLCONFIG_LIBRARY_OS_ENTRYFUNCNAME_QUOTE));

  klapi_pop(state, 1);  /* pop corelibpath */

  kl_assert(klstack_size(klstate_stack(state)) == 0, "");

  /* create actual error handler */
  KLAPI_PROTECT(klapi_pushstring(state, "traceback"));
  klapi_pushnil(state, 1);
  klapi_pushcfunc(state, btool->traceback);
  KLAPI_PROTECT(klapi_mkcclosure(state, -2, errhandler, 1));
  KLAPI_PROTECT(klapi_storeglobal(state, klapi_getstring(state, -3), -2));
  klapi_popclose(state, 3);
  return 0;
}

static KlException call_bcloader(KlState* state, Ki* input, Ko* err, KlCFunction* bcloader) {
  KLAPI_PROTECT(klapi_checkstack(state, 2));

  klapi_pushuserdata(state, input); /* first argument: input stream */
  klapi_pushuserdata(state, err);   /* second argument: error stream */

  /* call bcloader */
  KlValue callable;
  klvalue_setcfunc(&callable, bcloader);
  KLAPI_PROTECT(klapi_tryscall(state, -3, &callable, 2, 1));
  return KL_E_NONE;
}

static KlException call_compiler(KlState* state, Ki* input, Ko* err, const char* inputname, const char* srcfile, KlCFunction* compiler) {
  KLAPI_PROTECT(klapi_checkstack(state, 4));

  klapi_pushuserdata(state, input); /* first argument: input stream */
  klapi_pushuserdata(state, err);   /* second argument: error stream */
  /* third argument: input name */
  KLAPI_PROTECT(klapi_pushstring(state, inputname));
  /* fourth argument: source file path */
  if (srcfile) {
    KLAPI_PROTECT(klapi_pushstring(state, srcfile));
  } else {
    klapi_pushnil(state, 1);
  }

  /* call compiler */
  KlValue callable;
  klvalue_setcfunc(&callable, compiler);
  KLAPI_PROTECT(klapi_tryscall(state, -5, &callable, 4, 1));
  return KL_E_NONE;
}

static KlException execute_script(Behaviour* behaviour, KlState* state, BasicTool* btool, Ko* err) {
  Ki* input = NULL;
  /* create input stream */
  if (behaviour->option & OPTION_IN_ARG) {
    input = kibuf_create(behaviour->stmt, strlen(behaviour->stmt));
    if (kl_unlikely(!input))
      return klapi_throw_internal(state, KL_E_OOM, "can not create input stream");
  } else {
    kl_assert(behaviour->option & OPTION_IN_FILE, "");
    input = kifile_create(behaviour->filename, "rb");
    if (kl_unlikely(!input))
      return klapi_throw_internal(state, KL_E_OOM, "can not open file: %s", behaviour->filename);
  }

  /* push error handler */
  KLAPI_MAYFAIL(klapi_checkstack(state, 1), ki_delete(input));
  KLAPI_MAYFAIL(klapi_pushstring(state, "traceback"), ki_delete(input));
  klapi_loadglobal(state);  /* error handler: traceback */

  /* call compiler */
  if (behaviour->option & OPTION_UNDUMP) {
    kl_assert(behaviour->option & OPTION_IN_FILE, "");
    KLAPI_MAYFAIL(call_bcloader(state, input, err, btool->bcloader), ki_delete(input));
  } else {
    KLAPI_MAYFAIL(call_compiler(state, input, err, 
                                   (behaviour->option & OPTION_IN_ARG) ? "command line" : behaviour->filename,
                                   (behaviour->option & OPTION_IN_ARG) ? NULL : behaviour->filename,
                                   btool->compiler),
                  ki_delete(input));
  }
  ki_delete(input);

  if (klapi_checktype(state, -1, KL_NIL)) {
    klapi_pop(state, 2);  /* pop result and error handler */
    return 0;
  }

  /* push arguments to script */
  KLAPI_PROTECT(klapi_checkstack(state, behaviour->narg));
  for (size_t i = 0; i < behaviour->narg; ++i) {
    KLAPI_PROTECT(klapi_pushstring(state, behaviour->args[i]));
  }
  /* execute script */
  KLAPI_PROTECT(klapi_tryscall(state, -2 - behaviour->narg, klapi_access(state, -1 - behaviour->narg), behaviour->narg, 0));
  klapi_pop(state, 2);                /* pop klang closure and error handler */
  return 0;
}

static KlException interactive(KlState* state, BasicTool* btool, Ko* err) {
  KLAPI_PROTECT(klapi_checkstack(state, 1));
  KLAPI_PROTECT(klapi_pushstring(state, "traceback"));
  klapi_loadglobal(state);  /* error handler: traceback */
  while (true) {
    Ki* input = interactive_create();
    if (kl_unlikely(!input))
      return klapi_throw_internal(state, KL_E_OOM, "can not create input stream");

    /* call compiler */
    KLAPI_MAYFAIL(call_compiler(state, input, err, "stdin", NULL, btool->compileri),
                  ki_delete(input));

    bool eof = klcast(KiInteractive*, input)->eof;
    ki_delete(input);
    if (klapi_checktype(state, -1, KL_NIL)) {
      klapi_pop(state, 1);
      if (eof) return KL_E_NONE;
      continue;
    }

    /* call */
    ptrdiff_t stktop_save = klexec_savestack(state, klstate_stktop(state));
    KLAPI_PROTECT(klapi_tryscall(state, -2, klapi_access(state, -1), 0, KLAPI_VARIABLE_RESULTS));
    int nres = klexec_savestack(state, klstate_stktop(state)) - stktop_save;
    if (nres != 0) {
      /* print results */
      KLAPI_PROTECT(klapi_checkstack(state, 1));
      KLAPI_PROTECT(klapi_pushstring(state, "print"));
      klapi_loadglobal(state);
      klapi_setvalue(state, -nres - 2, klapi_access(state, -1));
      klapi_pop(state, 1);
      KLAPI_PROTECT(klapi_tryscall(state, -nres - 2, klapi_access(state, -nres - 1), nres, 0));
    }
    klapi_pop(state, 1);
    kl_assert(klstack_size(klstate_stack(state)) == 1, "");
    if (eof) return KL_E_NONE;
  }
}

static KlException errhandler(KlState* state) {
  KlException exception = klapi_currexception(state);
  if (exception == KL_E_USER) {
    fprintf(stderr, "|| user defined exception currently is not completely supported\n");
    klapi_throw_internal(state, KL_E_NONE, "");
    KlValue* traceback = klapi_getref(state, 0);
    return klapi_tailcall(state, traceback, 0);
  }

  if (exception == KL_E_LINK) {
    KlState* costate = klstate_exception_source(state);
    klco_setstatus(&costate->coinfo, KLCO_RUNNING);
    klco_allow_yield(&costate->coinfo, false);
    KLAPI_MAYFAIL(klapi_scall(costate, &klvalue_obj(klstate_currci(state)->callable.clo, KL_CCLOSURE), 0, 0),
                  klco_setstatus(&costate->coinfo, KLCO_DEAD));
    klco_setstatus(&costate->coinfo, KLCO_DEAD);
    fprintf(stderr, "::\n");
    fprintf(stderr, "|| exception thrown across coroutine(see above). the coroutine is called from here:\n");
  } else {
    fprintf(stderr, "|| exception message: %s\n", klapi_exception_message(state));
  }

  klapi_throw_internal(state, KL_E_NONE, "");
  KlValue* traceback = klapi_getref(state, 0);
  return klapi_tailcall(state, traceback, 0);
}

static bool matchraw(const char* arg, unsigned nopts, ...) {
  va_list opts;
  va_start(opts, nopts);
  for (unsigned i = 0; i < nopts; ++i) { 
    if (strcmp(va_arg(opts, const char*), arg) == 0) {
      va_end(opts);
      return true;
    }
  }
  va_end(opts);
  return false;
}

static void cleanbehaviour(Behaviour* behaviour) {
  if (behaviour->option & OPTION_IN_FILE)
    free(behaviour->filename);
  if (behaviour->corelibpath)
    free(behaviour->corelibpath);
}

static int validatebehaviour(Behaviour* behaviour) {
  if (!(behaviour->option & (OPTION_IN_FILE | OPTION_IN_ARG)))
    behaviour->option |= OPTION_INTER;

  if (!behaviour->corelibpath) {
#ifdef CORELIBPATH
    behaviour->corelibpath = strdup(CORELIBPATH);
#else
    behaviour->corelibpath = strdup("/usr/local/lib/klang");
#endif
  }
  return 0;
}

static void print_help(void) {
  printf("Usage: klang [options] [(<script> | -e <code> | -u <filename>) [args...]]\n");
  printf("options:\n");
  printf("  -h --help                   show this message.\n");
  printf("  -e <code>                   execute the code provided by command line.\n");
  printf("  -u --undump <filename>      load byte code from specified file.\n");
  printf("  -i --interactive            always enter interactive mode.\n");
  printf("  --corelibpath               where to load the core library.\n");
  printf("if input is not specified, enter interactive mode.\n");
}

static int fatalerror(const char* stage, const char* fmt, ...) {
  fprintf(stderr, "fatal error while %s: ", stage);
  va_list args;
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  fputc('\n', stderr);
  va_end(args);
  return 1;
}


static void interactive_delete(KiInteractive* ki) {
  free(ki->buf);
  free(ki);
}

static void interactive_reader(KiInteractive* ki) {
  KioFileOffset readpos = ki_tell((Ki*)ki);
  if (readpos < (KioFileOffset)(ki->curr - ki->buf)) {
    ki_setbuf((Ki*)ki, ki->buf + readpos, ki->curr - (ki->buf + readpos), readpos);
    return;
  }
  if (ki->eof) {
    ki_setbuf((Ki*)ki, ki->buf, 0, readpos);
    return;
  }
  printf(ki->buf ? "... " : ">>> ");
  if (readpos >= (KioFileOffset)(ki->end - ki->buf)) {
    size_t oldsize = ki->curr - ki->buf;
    size_t newcap = readpos + 8096;
    unsigned char* newbuf = realloc(ki->buf, newcap);
    if (kl_unlikely(!newbuf)) {
      ki_setbuf((Ki*)ki, ki->curr, 0, readpos);
      return;
    }
    ki->end = newbuf + newcap;
    ki->curr = newbuf + oldsize;
    ki->buf = newbuf;
  }
  if (fgets((char*)ki->curr, ki->end - ki->curr, stdin) == NULL) {
    ki->eof = true;
    ki_setbuf((Ki*)ki, ki->curr, 0, readpos);
    putchar('\n');  /* user ends the interactive mode, print a newline */
    return;
  }
  size_t readsize = strlen((char*)ki->curr);
  ki->curr += readsize;
  if (readpos >= (KioFileOffset)(ki->curr - ki->buf)) {
    ki_setbuf((Ki*)ki, ki->curr, 0, readpos);
    return;
  }
  ki_setbuf((Ki*)ki, ki->buf + readpos, ki->curr - (ki->buf + readpos), readpos);
  return;
}

static Ki* interactive_create(void) {
  KiInteractive* input = (KiInteractive*)malloc(sizeof (KiInteractive));
  if (kl_unlikely(!input)) return NULL;
  input->buf = NULL;
  input->curr = NULL;
  input->end = NULL;
  input->eof = false;
  ki_init(klcast(Ki*, input), &interactive_vfunc);
  return klcast(Ki*, input);
}
