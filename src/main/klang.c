#include "include/kio/ki.h"
#include "include/klapi.h"
#include "include/kio/kibuf.h"
#include "include/kio/kifile.h"
#include "include/kio/kofile.h"
#include "deps/k/include/os_spec/kfs.h"
#include "include/vm/klexception.h"
#include "include/vm/klexec.h"
#include <stdarg.h>

#define KL_OPTION_NORMAL    ((unsigned)0)
#define KL_OPTION_HELP      ((unsigned)klbit(0))
#define KL_OPTION_INTER     ((unsigned)klbit(1))
#define KL_OPTION_IN_FILE   ((unsigned)klbit(2))
#define KL_OPTION_IN_ARG    ((unsigned)klbit(3))


#define KL_NARGRAW(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, ...)     (_9)
#define KL_NARG(...)        KL_NARGRAW(__VA_ARGS__ , 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)
#define kl_match(opt, ...)  kl_matchraw((opt), KL_NARG(__VA_ARGS__), __VA_ARGS__)
#define kl_isfilename(arg)  ((arg)[0] != '-')

typedef struct tagKlBehaviour {
  union {
    char* filename;
    const char* stmt;
  };
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
static int kl_dopreload(KlBehaviour* behaviour, KlState* state, KlBasicTool* btool);
static int kl_do_script(KlBehaviour* behaviour, KlState* state, KlBasicTool* btool, Ko* err);
static int kl_interactive(KlState* state, KlBasicTool* btool, Ko* err);

static KlException kl_errhandler(KlState* state);

int main(int argc, char** argv) {
  KlBehaviour behaviour;
  int ret = kl_parse_argv(argc, argv, &behaviour);
  if (kl_unlikely(ret)) return ret;
  return kl_do(&behaviour);
}

static int kl_parse_argv(int argc, char** argv, KlBehaviour* behaviour) {
  behaviour->filename = NULL;
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
  KlMM klmm;
  klmm_init(&klmm, 32 * 1024);
  KlState* state = klapi_new_state(&klmm);
  if (kl_unlikely(!state)) {
    klmm_destroy(&klmm);
    return kl_fatalerror("initializing", "can not create virtual machine");
  }
  KlBasicTool btool = { NULL, NULL, NULL, NULL };
  int ret = kl_dopreload(behaviour, state, &btool);
  if (kl_unlikely(ret)) {
    klmm_destroy(&klmm);
    kl_cleanbehaviour(behaviour);
    return ret;
  }
  Ko* err = kofile_attach(stderr);
  if (kl_unlikely(!err)) {
    klmm_destroy(&klmm);
    kl_cleanbehaviour(behaviour);
    return kl_fatalerror("initializing", "can not create error output stream");
  }
  ret = kl_do_script(behaviour, state, &btool, err);
  if (kl_unlikely(ret)) {
    klmm_destroy(&klmm);
    kl_cleanbehaviour(behaviour);
    ko_delete(err);
    return ret;
  }
  ret = kl_interactive(state, &btool, err);
  klmm_destroy(&klmm);
  kl_cleanbehaviour(behaviour);
  ko_delete(err);
  return ret;
}

static int kl_dopreload(KlBehaviour* behaviour, KlState* state, KlBasicTool* btool) {
  kl_unused(behaviour);
  KlException exception = klapi_loadlib(state, "./runtime_compiler.so", NULL);
  if (kl_unlikely(exception))
    return kl_fatalerror("loading basic libraries", "can not load neccessary library: runtime_compiler.so");
  exception = klapi_loadlib(state, "./traceback.so", NULL);
  if (kl_unlikely(exception))
    return kl_fatalerror("loading basic libraries", "can not load neccessary library: traceback.so");
  exception = klapi_loadlib(state, "./basic.so", NULL);
  if (kl_unlikely(exception))
    return kl_fatalerror("loading basic libraries", "can not load neccessary library: basic.so");
  btool->traceback = klapi_getcfunc(state, -1);
  btool->bcloader = klapi_getcfunc(state, -2);
  btool->compileri = klapi_getcfunc(state, -3);
  btool->compiler = klapi_getcfunc(state, -4);
  klapi_pop(state, 4);
  kl_assert(klstack_size(klstate_stack(state)) == 0, "");
  klapi_pushstring(state, "traceback");
  klapi_pushcfunc(state, btool->traceback);
  if (kl_unlikely(klapi_mkcclosure(state, -1, kl_errhandler, 1)))
    return kl_fatalerror("making error handler", "can not make error handler");
  if (kl_unlikely(klapi_storeglobal(state, klapi_getstring(state, -2), -1)))
    return kl_fatalerror("making error handler", "can not set global variable: traceback");
  klapi_pop(state, 2);
  return 0;
}

static int kl_do_script(KlBehaviour* behaviour, KlState* state, KlBasicTool* btool, Ko* err) {
  Ki* input = NULL;
  /* create input stream */
  if (behaviour->option & KL_OPTION_IN_ARG) {
    input = kibuf_create(behaviour->stmt, strlen(behaviour->stmt));
    if (kl_unlikely(!input))
      return kl_fatalerror("executing script", "failed to create input stream");
  } else {
    kl_assert(behaviour->option & KL_OPTION_IN_FILE, "");
    input = kifile_create(behaviour->filename);
    if (kl_unlikely(!input))
      return kl_fatalerror("executing script", "can not open file: ", behaviour->filename);
  }

  /* push arguments to compiler */
  KlException exception = klapi_checkstack(state, 5);
  if (kl_unlikely(exception)) {
    ki_delete(input);
    return kl_fatalerror("executing script", "can not execute script");
  }
  exception = klapi_pushstring(state, "traceback");
  if (kl_unlikely(exception)) {
    ki_delete(input);
    return kl_fatalerror("executing script", "can not execute script");
  }
  exception = klapi_loadglobal(state);  /* error handler: traceback */
  if (kl_unlikely(exception)) {
    ki_delete(input);
    return kl_fatalerror("executing script", "can not load global: traceback");
  }
  klapi_pushuserdata(state, input); /* first argument: input stream */
  klapi_pushuserdata(state, err);   /* second argument: error stream */
  /* third argument: input name */
  exception = klapi_pushstring(state, (behaviour->option & KL_OPTION_IN_ARG) ? "command line" : behaviour->filename);
  if (kl_unlikely(exception)) {
    ki_delete(input);
    return kl_fatalerror("executing script", "can not execute script");
  }
  /* fourth argument: source file path */
  if (behaviour->option & KL_OPTION_IN_ARG) {
    klapi_pushnil(state, 1);
  } else {
    if (kl_unlikely(klapi_pushstring(state, behaviour->filename))) {
      ki_delete(input);
      return kl_fatalerror("executing script", "can not execute script");
    }
  }
  KlValue callable;
  klvalue_setcfunc(&callable, btool->compiler);
  exception = klapi_tryscall(state, &callable, 4, 1);
  ki_delete(input);
  if (kl_unlikely(exception))
    return kl_fatalerror("executing script", "exception occurred while handling exception");
  if (klapi_checktype(state, -1, KL_NIL)) {
    klapi_pop(state, 1);
    return 0;
  }
  klapi_tryscall(state, klapi_access(state, -1), 0, 0);
  if (kl_unlikely(exception))
    return kl_fatalerror("executing script", "exception occurred while handling exception");
  return 0;
}

static int kl_interactive(KlState* state, KlBasicTool* btool, Ko* err) {
  while (true) {
    Ki* input = kl_interactive_create();
    if (kl_unlikely(!input))
      return kl_fatalerror("doing interactive", "can not create input stream");
    /* push arguments to compiler */
    klapi_pushuserdata(state, input); /* first argument: input stream */
    klapi_pushuserdata(state, err);   /* second argument: error stream */
    /* third argument: input name */
    KlException exception = klapi_pushstring(state, "stdin");
    if (kl_unlikely(exception)) {
      ki_delete(input);
      return kl_fatalerror("doing interactive", "can not create string: \"stdin\"");
    }
    /* fourth argument: source file path */
    klapi_pushnil(state, 1);
    KlValue callable;
    klvalue_setcfunc(&callable, btool->compileri);
    exception = klapi_tryscall(state, &callable, 4, 1);
    ki_delete(input);
    if (kl_unlikely(exception)) {
      return kl_fatalerror("doing interactive", "exception occurred while handling exception");
    }
    if (klapi_checktype(state, -1, KL_NIL)) {
      klapi_pop(state, 1);
      continue;
    }
    ptrdiff_t stktop_save = klexec_savestack(state, klstate_stktop(state));
    exception = klapi_tryscall(state, klapi_access(state, -1), 0, KLAPI_VARIABLE_RESULTS);
    if (kl_unlikely(exception))
      return kl_fatalerror("doing interactive", "exception occurred while handling exception");
    int nres = klexec_savestack(state, klstate_stktop(state)) - stktop_save;
    if (nres == 0) {
      klapi_pop(state, 1);
      continue;
    }
    exception = klapi_pushstring(state, "print");
    if (kl_unlikely(exception))
      return kl_fatalerror("doing interactive", "failed to call function 'print'");
    klapi_loadglobal(state);
    klapi_setvalue(state, -nres - 2, klapi_access(state, -1));
    klapi_pop(state, 1);
    exception = klapi_tryscall(state, klapi_access(state, -nres - 1), nres, 0);
    if (kl_unlikely(exception))
      return kl_fatalerror("doing interactive", "exception occurred while handling exception");
    klapi_pop(state, 1);
  }
}

static KlException kl_errhandler(KlState* state) {
  KlException exception = klapi_currexception(state);
  if (exception == KL_E_USER || exception == KL_E_LINK) {
    fprintf(stderr, "||-user defined exception and exception thrown across coroutine is currently not completely supported\n");
  } else {
    fprintf(stderr, "||-exception message: %s\n", klapi_exception_message(state));
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
  if (behaviour->option & KL_OPTION_IN_FILE) {
    free(behaviour->filename);
  }
}

static int kl_validatebehaviour(KlBehaviour* behaviour) {
  if (behaviour->option & (KL_OPTION_IN_FILE | KL_OPTION_IN_ARG))
    return 0;
  behaviour->option |= KL_OPTION_INTER;
  return 0;
}

static void kl_print_help(void) {
  printf("Usage: klang [<inputfile> | -e statement] [options]\n");
  printf("options:\n");
  printf("  -h --help                   show this message.\n");
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
  if (readpos < (KioFileOffset)(ki->curr - ki->buf) || ki->eof) {
    ki_setbuf((Ki*)ki, ki->buf + readpos, ki->curr - (ki->buf + readpos), readpos);
    return;
  }
  printf(ki->buf ? ">>>>" : ">>");
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
  fgets((char*)ki->curr, ki->end - ki->curr, stdin);
  size_t readsize = strlen((char*)ki->curr);
  ki->eof = feof(stdin);
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
