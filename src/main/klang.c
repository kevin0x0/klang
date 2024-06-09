#include "include/kio/ki.h"
#include "include/klapi.h"
#include "include/kio/kibuf.h"
#include "include/kio/kifile.h"
#include "include/kio/kofile.h"
#include "deps/k/include/os_spec/kfs.h"
#include "include/string/kstring.h"
#include "include/vm/klexception.h"
#include "include/vm/klexec.h"
#include <stdarg.h>

#define KL_OPTION_NORMAL    ((unsigned)0)
#define KL_OPTION_HELP      ((unsigned)klbit(0))
#define KL_OPTION_INTER     ((unsigned)klbit(1))
#define KL_OPTION_IN_FILE   ((unsigned)klbit(2))
#define KL_OPTION_IN_ARG    ((unsigned)klbit(3))
#define KL_OPTION_UNDUMP    ((unsigned)klbit(4))


#define KL_NARGRAW(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, ...)     (_9)
#define KL_NARG(...)        KL_NARGRAW(__VA_ARGS__ , 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)
#define kl_match(opt, ...)  kl_matchraw((opt), KL_NARG(__VA_ARGS__), __VA_ARGS__)
#define kl_isfilename(arg)  ((arg)[0] != '-')

typedef struct tagKlBehaviour {
  union {
    char* filename;
    const char* stmt;
  };
  char* corelibpath;
  char** args;
  size_t narg;
  unsigned option;
} KlBehaviour;

typedef struct tagKlBasicTool {
  KlCFunction* bcloader;
  KlCFunction* compiler;
  KlCFunction* compileri;
  KlCFunction* traceback;
} KlBasicTool;

typedef struct tagKiInteractive {
  Ki base;
  unsigned char* buf;
  unsigned char* curr;
  unsigned char* end;
  bool eof;
} KiInteractive;

static void kl_interactive_input_delete(KiInteractive* ki);
static void kl_interactive_input_reader(KiInteractive* ki);
static Ki* kl_interactive_create(void);

static KiVirtualFunc kl_interactive_input_vfunc = {
  .size = NULL,
  .delete = (KiDelete)kl_interactive_input_delete,
  .reader = (KiReader)kl_interactive_input_reader,
};

static int kl_parse_argv(int argc, char** argv, KlBehaviour* behaviour);
static bool kl_matchraw(const char* arg, unsigned nopts, ...);
static void kl_cleanbehaviour(KlBehaviour* behaviour);
static int kl_validatebehaviour(KlBehaviour* behaviour);
static void kl_print_help(void);

static int kl_fatalerror(const char* stage, const char* fmt, ...);

static int kl_do(KlBehaviour* behaviour);
static KlException kl_dopreload(KlBehaviour* behaviour, KlState* state, KlBasicTool* btool);
static KlException kl_call_bcloader(KlState* state, Ki* input, Ko* err, KlCFunction* bcloader);
static KlException kl_call_compiler(KlState* state, Ki* input, Ko* err, const char* inputname, const char* srcfile, KlCFunction* compiler);
static KlException kl_do_script(KlBehaviour* behaviour, KlState* state, KlBasicTool* btool, Ko* err);
static KlException kl_interactive(KlState* state, KlBasicTool* btool, Ko* err);

static KlException kl_errhandler(KlState* state);

int main(int argc, char** argv) {
  KlBehaviour behaviour;
  int ret = kl_parse_argv(argc, argv, &behaviour);
  if (kl_unlikely(ret)) return ret;
  return kl_do(&behaviour);
}

static int kl_parse_argv(int argc, char** argv, KlBehaviour* behaviour) {
  behaviour->filename = NULL;
  behaviour->corelibpath = NULL;
  behaviour->args = NULL;
  behaviour->narg = 0;
  behaviour->stmt = NULL;
  behaviour->option = KL_OPTION_NORMAL;
  for (int i = 1; i < argc; ++i) {
    if (kl_match(argv[i], "-h", "--help")) {
      behaviour->option |= KL_OPTION_HELP;
    } else if (kl_match(argv[i], "-e")) {
      if (behaviour->option & (KL_OPTION_IN_FILE | KL_OPTION_IN_ARG)) {
        fprintf(stderr, "the input is specified more than once: %s.\n", argv[i]);
        kl_cleanbehaviour(behaviour);
        return 1;
      }
      if (!argv[++i]) {
        fprintf(stderr, "expected to be executed statement after %s.\n", argv[i - 1]);
        kl_cleanbehaviour(behaviour);
        return 1;
      }
      behaviour->option |= KL_OPTION_IN_ARG;
      behaviour->stmt = argv[i++];
      behaviour->args = argv + i;
      behaviour->narg = argc - i;
      break;
    } else if (kl_match(argv[i], "-i", "--interactive")) {
      behaviour->option |= KL_OPTION_INTER;
    } else if (kl_match(argv[i], "-u", "--undump")) {
      if (++i == argc) {
        fprintf(stderr, "expected <filename> after option: %s", argv[i - 1]);
        kl_cleanbehaviour(behaviour);
        return 1;
      }
      if (behaviour->option & (KL_OPTION_IN_FILE | KL_OPTION_IN_ARG)) {
        fprintf(stderr, "the input is specified more than once: %s.\n", argv[i]);
        kl_cleanbehaviour(behaviour);
        return 1;
      }
      behaviour->option |= KL_OPTION_IN_FILE | KL_OPTION_UNDUMP;
      if (!(behaviour->filename = kfs_abspath(argv[i]))) {
        fprintf(stderr, "out of memory while parsing command line\n");
        kl_cleanbehaviour(behaviour);
        return 1;
      }
      behaviour->args = argv + ++i;
      behaviour->narg = argc - i;
      break;
    } else if (kl_match(argv[i], "--corelibpath")) {
      if (++i == argc) {
        fprintf(stderr, "expected <directory path> after option: %s", argv[i - 1]);
        kl_cleanbehaviour(behaviour);
        return 1;
      }
      if (behaviour->corelibpath) {
        fprintf(stderr, "the core library path is specified more than once: %s.\n", argv[i]);
        kl_cleanbehaviour(behaviour);
        return 1;
      }
      if (!(behaviour->corelibpath = kstr_copy(argv[i]))) {
        fprintf(stderr, "out of memory while parsing command line\n");
        kl_cleanbehaviour(behaviour);
        return 1;
      }
      size_t len = strlen(behaviour->corelibpath);
      if (len != 0 && (behaviour->corelibpath[len - 1] == '/'
                   || behaviour->corelibpath[len - 1] == '\\')) {
        behaviour->corelibpath[len - 1] = '\0';
      }
    } else if (kl_isfilename(argv[i])) {
      if (behaviour->option & (KL_OPTION_IN_FILE | KL_OPTION_IN_ARG)) {
        fprintf(stderr, "the input is specified more than once: %s.\n", argv[i]);
        kl_cleanbehaviour(behaviour);
        return 1;
      }
      behaviour->option |= KL_OPTION_IN_FILE;
      if (!(behaviour->filename = kfs_abspath(argv[i]))) {
        fprintf(stderr, "out of memory while parsing command line\n");
        kl_cleanbehaviour(behaviour);
        return 1;
      }
      behaviour->args = argv + ++i;
      behaviour->narg = argc - i;
      break;
    } else {
      fprintf(stderr, "unrecognized option: %s\n", argv[i]);
      kl_print_help();
      kl_cleanbehaviour(behaviour);
      return 1;
    }
  }
  return kl_validatebehaviour(behaviour);
}

static int kl_do(KlBehaviour* behaviour) {
  if (behaviour->option & KL_OPTION_HELP) {
    kl_print_help();
    kl_cleanbehaviour(behaviour);
    return 0;
  }
  int failure = 0;
  KlMM klmm;
  klmm_init(&klmm, 32 * 1024);
  KlState* state = klapi_new_state(&klmm);
  if (kl_unlikely(!state)) {
    failure = kl_fatalerror("initializing", "can not create virtual machine");
    goto error_create_vm;
  }
  KlBasicTool btool = { NULL, NULL, NULL, NULL };
  failure = kl_dopreload(behaviour, state, &btool);
  if (kl_unlikely(failure)) {
    fprintf(stderr, "%s\n", klapi_exception_message(state));
    goto error_preload;
  }
  Ko* err = kofile_attach(stderr);
  if (kl_unlikely(!err)) {
    failure = kl_fatalerror("initializing", "can not create error output stream");
    goto error_create_err;
  }
  if (behaviour->option & (KL_OPTION_IN_ARG | KL_OPTION_IN_FILE)) {
    failure = kl_do_script(behaviour, state, &btool, err);
    if (kl_unlikely(failure)) {
      fprintf(stderr, "%s\n", klapi_exception_message(state));
      goto error_do_script;
    }
  }
  if (behaviour->option & KL_OPTION_INTER) {
    failure = kl_interactive(state, &btool, err);
    if (kl_unlikely(failure))
      fprintf(stderr, "%s\n", klapi_exception_message(state));
  }

error_do_script:
  ko_delete(err);
error_create_err:
error_preload:
  klmm_destroy(&klmm);
error_create_vm:
  kl_cleanbehaviour(behaviour);
  return failure;
}

static KlException kl_dopreload_helper_loadlib(KlState* state, const char* libname) {
  KLAPI_PROTECT(klapi_checkstack(state, 1));
  KLAPI_PROTECT(klapi_pushstring(state,libname));
  KLAPI_PROTECT(klapi_concati(state, -1, -2, -1));
  KLAPI_PROTECT(klapi_loadlib(state, klstring_content(klapi_getstring(state, -1)), NULL));
  return KL_E_NONE;
}

static KlException kl_dopreload(KlBehaviour* behaviour, KlState* state, KlBasicTool* btool) {
  kl_unused(behaviour);
  KLAPI_PROTECT(klapi_checkstack(state, 1));
  KLAPI_PROTECT(klapi_pushstring(state, behaviour->corelibpath));

  /* load compiler */
  KLAPI_PROTECT(kl_dopreload_helper_loadlib(state, "/runtime_compiler.so"));
  btool->compiler = klapi_getcfunc(state, -3);
  btool->compileri = klapi_getcfunc(state, -2);
  btool->bcloader = klapi_getcfunc(state, -1);
  klapi_pop(state, 4); /* pop 3 results and 1 string */

  /* load traceback */
  KLAPI_PROTECT(kl_dopreload_helper_loadlib(state, "/traceback.so"));
  btool->traceback = klapi_getcfunc(state, -1);
  klapi_pop(state, 2); /* pop 1 result and 1 string */

  /* load basic */
  KLAPI_PROTECT(kl_dopreload_helper_loadlib(state, "/basic.so"));
  klapi_pop(state, 1); /* pop 1 string */

  klapi_pop(state, 1);  /* pop corelibpath */

  kl_assert(klstack_size(klstate_stack(state)) == 0, "");

  /* create actual error handler */
  KLAPI_PROTECT(klapi_pushstring(state, "traceback"));
  klapi_pushnil(state, 1);
  klapi_pushcfunc(state, btool->traceback);
  KLAPI_PROTECT(klapi_mkcclosure(state, -2, kl_errhandler, 1));
  KLAPI_PROTECT(klapi_storeglobal(state, klapi_getstring(state, -3), -2));
  klapi_popclose(state, 3);
  return 0;
}

static KlException kl_call_bcloader(KlState* state, Ki* input, Ko* err, KlCFunction* bcloader) {
  KLAPI_PROTECT(klapi_checkstack(state, 2));

  klapi_pushuserdata(state, input); /* first argument: input stream */
  klapi_pushuserdata(state, err);   /* second argument: error stream */

  /* call bcloader */
  KlValue callable;
  klvalue_setcfunc(&callable, bcloader);
  KLAPI_PROTECT(klapi_tryscall(state, -3, &callable, 2, 1));
  return KL_E_NONE;
}

static KlException kl_call_compiler(KlState* state, Ki* input, Ko* err, const char* inputname, const char* srcfile, KlCFunction* compiler) {
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

static KlException kl_do_script(KlBehaviour* behaviour, KlState* state, KlBasicTool* btool, Ko* err) {
  Ki* input = NULL;
  /* create input stream */
  if (behaviour->option & KL_OPTION_IN_ARG) {
    input = kibuf_create(behaviour->stmt, strlen(behaviour->stmt));
    if (kl_unlikely(!input))
      return klapi_throw_internal(state, KL_E_OOM, "can not create input stream");
  } else {
    kl_assert(behaviour->option & KL_OPTION_IN_FILE, "");
    input = kifile_create(behaviour->filename);
    if (kl_unlikely(!input))
      return klapi_throw_internal(state, KL_E_OOM, "can not open file: %s", behaviour->filename);
  }

  /* push error handler */
  KLAPI_MAYFAIL(klapi_checkstack(state, 1), ki_delete(input));
  KLAPI_MAYFAIL(klapi_pushstring(state, "traceback"), ki_delete(input));
  klapi_loadglobal(state);  /* error handler: traceback */

  /* call compiler */
  if (behaviour->option & KL_OPTION_UNDUMP) {
    kl_assert(behaviour->option & KL_OPTION_IN_FILE, "");
    KLAPI_MAYFAIL(kl_call_bcloader(state, input, err, btool->bcloader), ki_delete(input));
  } else {
    KLAPI_MAYFAIL(kl_call_compiler(state, input, err, 
                                   (behaviour->option & KL_OPTION_IN_ARG) ? "command line" : behaviour->filename,
                                   (behaviour->option & KL_OPTION_IN_ARG) ? NULL : behaviour->filename,
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

static KlException kl_interactive(KlState* state, KlBasicTool* btool, Ko* err) {
  KLAPI_PROTECT(klapi_checkstack(state, 1));
  KLAPI_PROTECT(klapi_pushstring(state, "traceback"));
  klapi_loadglobal(state);  /* error handler: traceback */
  while (true) {
    Ki* input = kl_interactive_create();
    if (kl_unlikely(!input))
      return klapi_throw_internal(state, KL_E_OOM, "can not create input stream");

    /* call compiler */
    KLAPI_MAYFAIL(kl_call_compiler(state, input, err, "stdin", NULL, btool->compileri),
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

static KlException kl_errhandler(KlState* state) {
  KlException exception = klapi_currexception(state);
  if (exception == KL_E_USER || exception == KL_E_LINK) {
    fprintf(stderr, "|| user defined exception and exception thrown across coroutine is currently not completely supported\n");
  } else {
    fprintf(stderr, "|| exception message: %s\n", klapi_exception_message(state));
  }
  klapi_throw_internal(state, KL_E_NONE, "");
  KlValue* traceback = klapi_getref(state, 0);
  return klapi_tailcall(state, traceback, 0);
}

static bool kl_matchraw(const char* arg, unsigned nopts, ...) {
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

static void kl_cleanbehaviour(KlBehaviour* behaviour) {
  if (behaviour->option & KL_OPTION_IN_FILE)
    free(behaviour->filename);
  if (behaviour->corelibpath)
    free(behaviour->corelibpath);
}

static int kl_validatebehaviour(KlBehaviour* behaviour) {
  if (!(behaviour->option & (KL_OPTION_IN_FILE | KL_OPTION_IN_ARG)))
    behaviour->option |= KL_OPTION_INTER;

  if (!behaviour->corelibpath) {
    if (!(behaviour->corelibpath = kfs_get_bin_dir())) {
      fprintf(stderr, "out of memory while validating behaviour\n");
      kl_cleanbehaviour(behaviour);
      return 1;
    }
    size_t pathlen = strlen(behaviour->corelibpath);
    kl_assert(pathlen > 4, "");
    strcpy(&behaviour->corelibpath[pathlen - 4], "lib/");
  }
  return 0;
}

static void kl_print_help(void) {
  printf("Usage: klang [options] [(<script> | -e <code> | -u <filename>) [args...]]\n");
  printf("options:\n");
  printf("  -h --help                   show this message.\n");
  printf("  -e <code>                   execute the code provided by command line.\n");
  printf("  -u --undump <filename>      load byte code from specified file.\n");
  printf("  -i --interactive            always enter interactive mode.\n");
  printf("  --corelibpath               where to load the core library.\n");
  printf("if input is not specified, enter interactive mode.\n");
}

static int kl_fatalerror(const char* stage, const char* fmt, ...) {
  fprintf(stderr, "fatal error while %s: ", stage);
  va_list args;
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  fputc('\n', stderr);
  va_end(args);
  return 1;
}


static void kl_interactive_input_delete(KiInteractive* ki) {
  free(ki->buf);
  free(ki);
}

static void kl_interactive_input_reader(KiInteractive* ki) {
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

static Ki* kl_interactive_create(void) {
  KiInteractive* input = (KiInteractive*)malloc(sizeof (KiInteractive));
  if (kl_unlikely(!input)) return NULL;
  input->buf = NULL;
  input->curr = NULL;
  input->end = NULL;
  input->eof = false;
  ki_init(klcast(Ki*, input), &kl_interactive_input_vfunc);
  return klcast(Ki*, input);
}
