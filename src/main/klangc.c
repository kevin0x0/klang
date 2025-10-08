#include "include/ast/klstrtbl.h"
#include "include/parse/kllex.h"
#include "include/parse/klparser.h"
#include "include/code/klcode.h"
#include "deps/k/include/kio/kifile.h"
#include "deps/k/include/kio/kofile.h"
#include "deps/k/include/os_spec/kfs.h"
#include <stdarg.h>

#define OPTION_NORMAL   ((unsigned)0)
#define OPTION_HELP     ((unsigned)klbit(0))
#define OPTION_PRINT    ((unsigned)klbit(1))
#define OPTION_DUMP     ((unsigned)klbit(2))
#define OPTION_UNDUMP   ((unsigned)klbit(3))
#define OPTION_STDIN    ((unsigned)klbit(4))


#define NARGRAW(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, ...)     (_9)
#define NARG(...)         NARGRAW(__VA_ARGS__ , 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)
#define match(opt, ...)   matchraw((opt), NARG(__VA_ARGS__), __VA_ARGS__)
#define isfilename(arg)   ((arg)[0] != '-')

typedef struct tagKlCBehaviour {
  Ki* input;
  const char* inputname;
  Ko* dumpoutput;
  Ko* textoutput;
  unsigned option;
} KlCBehaviour;

static int parse_argv(int argc, char** argv, KlCBehaviour* behaviour);
static KlCode* undump(KlCBehaviour* behaviour, KlStrTbl* strtbl);
static KlCode* compile(KlCBehaviour* behaviour, KlStrTbl* strtbl, KlError* klerr);
static KlCode* getcode(KlCBehaviour* behaviour, KlStrTbl* strtbl, KlError* klerr);
static int output(KlCBehaviour* behaviour, KlCode* code);
static int real_main(KlCBehaviour* behaviour);
static bool matchraw(const char* arg, unsigned nopts, ...);
static void cleanbehaviour(KlCBehaviour* behaviour);
static int validatebehaviour(KlCBehaviour* behaviour);
static void print_help(void);



int main(int argc, char** argv) {
  KlCBehaviour behaviour;
  int ret = parse_argv(argc, argv, &behaviour);
  if (kl_unlikely(ret)) return ret;
  return real_main(&behaviour);
}

static int parse_argv(int argc, char** argv, KlCBehaviour* behaviour) {
  behaviour->input = NULL;
  behaviour->inputname = NULL;
  behaviour->dumpoutput = NULL;
  behaviour->textoutput = NULL;
  behaviour->option = OPTION_NORMAL;
  if (argc == 1) {
    fprintf(stderr, "no argument\n");
    print_help();
    cleanbehaviour(behaviour);
    return 1;
  }
  for (int i = 1; i < argc; ++i) {
    if (match(argv[i], "-h", "--help")) {
      behaviour->option |= OPTION_HELP;
    } else if (match(argv[i], "-p", "--print")) {
      behaviour->option |= OPTION_PRINT;
      if (behaviour->textoutput)
        ko_delete(behaviour->textoutput);
      bool isfilename = argv[i + 1] && isfilename(argv[i + 1]);
      behaviour->textoutput = isfilename
        ? kofile_create(argv[i + 1], "wb")
        : kofile_attach(stdout);
      if (isfilename) ++i;
      if (kl_unlikely(!behaviour->textoutput)) {
        if (isfilename) {
          fprintf(stderr, "failed to open file: %s\n", argv[i]);
        } else {
          fprintf(stderr, "failed to attach to stdout\n");
        }
        cleanbehaviour(behaviour);
        return 1;
      }
    } else if (match(argv[i], "-d", "--dump")) {
      behaviour->option |= OPTION_DUMP;
      if (!(argv[i + 1] && isfilename(argv[i + 1]))) {
        fprintf(stderr, "%s should specify output file\n", argv[i]);
        cleanbehaviour(behaviour);
        return 1;
      }
      if (behaviour->dumpoutput)
        ko_delete(behaviour->dumpoutput);
      behaviour->dumpoutput = kofile_create(argv[++i], "wb");
      if (kl_unlikely(!behaviour->dumpoutput)) {
        fprintf(stderr, "failed to open file: %s\n", argv[i]);
        cleanbehaviour(behaviour);
        return 1;
      }
    } else if (match(argv[i], "-u", "--undump")) {
      behaviour->option |= OPTION_UNDUMP;
    } else if (isfilename(argv[i])) {
      if (behaviour->input) {
        fprintf(stderr, "the input is specified more than once\n");
        cleanbehaviour(behaviour);
        return 1;
      }
      if (kl_unlikely(!(behaviour->input = kifile_create(argv[i], "rb")))) {
        fprintf(stderr, "failed to open file: %s\n", argv[i]);
        cleanbehaviour(behaviour);
        return 1;
      }
      behaviour->inputname = argv[i];
    } else {
      fprintf(stderr, "unrecognized option: %s\n", argv[i]);
      print_help();
      cleanbehaviour(behaviour);
      return 1;
    }
  }
  return validatebehaviour(behaviour);
}

static KlCode* undump(KlCBehaviour* behaviour, KlStrTbl* strtbl) {
  KlUnDumpError error;
  KlCode* code = klcode_undump(behaviour->input, strtbl, &error);
  if (kl_likely(code)) return code;
  const char* errmsg = NULL;
  switch (error) {
    case KLUNDUMP_ERROR_BAD: {
      errmsg = "bad binary file";
      break;
    }
    case KLUNDUMP_ERROR_SIZE: {
      errmsg = "type size does not match";
      break;
    }
    case KLUNDUMP_ERROR_OOM: {
      errmsg = "out of memory";
      break;
    }
    case KLUNDUMP_ERROR_MAGIC: {
      errmsg = "not a bytecode file";
      break;
    }
    case KLUNDUMP_ERROR_ENDIAN: {
      errmsg = "byte order does not match";
      break;
    }
    case KLUNDUMP_ERROR_VERSION: {
      errmsg = "version does not match";
      break;
    }
    default: {
      errmsg = "unknown error";
      break;
    }
  }
  fprintf(stderr, "undump failure: %s\n", errmsg);
  return NULL;
}

static int getsrcfilename(KlCBehaviour* behaviour, KlStrTbl* strtbl, KlStrDesc* srcfile) {
  if (behaviour->option & OPTION_STDIN) {
    *srcfile = (KlStrDesc) { .id = 0, .length = 0 };
    return 0;
  }

  char* srcfileraw = kfs_abspath(behaviour->inputname);
  if (kl_unlikely(!srcfileraw)) return 1;

  size_t srcfilelen = strlen(srcfileraw);
  char* srcfile_intbl = klstrtbl_newstring(strtbl, srcfileraw);
  free(srcfileraw);
  if (kl_unlikely(!srcfile_intbl)) return 1;
  *srcfile = (KlStrDesc) { .id = klstrtbl_stringid(strtbl, srcfile_intbl), .length = srcfilelen };
  return 0;
}

static KlCode* compile(KlCBehaviour* behaviour, KlStrTbl* strtbl, KlError* klerr) {
  KlLex lex;
  if (kl_unlikely(!kllex_init(&lex, behaviour->input, klerr, behaviour->inputname, strtbl)))
    return NULL;
  KlParser parser;
  if (kl_unlikely(!klparser_init(&parser, strtbl, behaviour->inputname, klerr)))
    return NULL;

  /* parse */
  kllex_next(&lex);
  KlAstStmtList* ast = klparser_file(&parser, &lex);
  if (klerror_nerror(klerr) != 0) {
    if (ast) klast_delete(ast);
    kllex_destroy(&lex);
    return NULL;
  }

  KlStrDesc srcfilename;
  if (kl_unlikely(getsrcfilename(behaviour, strtbl, &srcfilename))) {
    if (ast) klast_delete(ast);
    kllex_destroy(&lex);
    return NULL;
  }

  /* genenrate code */
  KlCodeGenConfig config = {
    .inputname = behaviour->inputname,
    .srcfile = srcfilename,
    .input = behaviour->input,
    .klerr = klerr,
    .debug = false,
    .posinfo = true,
  };
  KlCode* code = klcode_create_fromast(ast, strtbl, &config);
  klast_delete(ast);
  kllex_destroy(&lex);
  if (klerror_nerror(klerr) != 0) {
    if (code) klcode_delete(code);
    return NULL;
  }
  return code;
}

static KlCode* getcode(KlCBehaviour* behaviour, KlStrTbl* strtbl, KlError* klerr) {
  return behaviour->option & OPTION_UNDUMP ? undump(behaviour, strtbl)
                                               : compile(behaviour, strtbl, klerr);
  
}

static int output(KlCBehaviour* behaviour, KlCode* code) {
  if (behaviour->option & OPTION_PRINT)
    klcode_print(code, behaviour->textoutput);
  if (behaviour->option & OPTION_DUMP) {
    if (kl_unlikely(!klcode_dump(code, behaviour->dumpoutput))) {
      fprintf(stderr, "dump failure\n");
      return 1;
    }
  }
  return 0;
}

static int real_main(KlCBehaviour* behaviour) {
  if (behaviour->option & OPTION_HELP) {
    print_help();
    cleanbehaviour(behaviour);
    return 0;
  }
  KlStrTbl* strtbl = klstrtbl_create();
  if (kl_unlikely(!strtbl))
    goto error_strtbl;
  Ko* errout = kofile_attach(stderr);
  if (kl_unlikely(!strtbl))
    goto error_errout;
  KlError klerr;
  klerror_init(&klerr, errout);

  int ret = 0;
  KlCode* code = getcode(behaviour, strtbl, &klerr);
  if (kl_unlikely(!code)) {
    ret = 1;
    goto getcode_error;
  }

  ret = output(behaviour, code);
  klcode_delete(code);
getcode_error:
  ko_delete(errout);
  klstrtbl_delete(strtbl);
  cleanbehaviour(behaviour);
  return ret;

error_errout:
  klstrtbl_delete(strtbl);
error_strtbl:
  fprintf(stderr, "fatal error: out of memory\n");
  cleanbehaviour(behaviour);
  return 1;
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

static void cleanbehaviour(KlCBehaviour* behaviour) {
  if (behaviour->textoutput) ko_delete(behaviour->textoutput);
  if (behaviour->dumpoutput) ko_delete(behaviour->dumpoutput);
  if (behaviour->input) ki_delete(behaviour->input);
  behaviour->textoutput = NULL;
  behaviour->dumpoutput = NULL;
  behaviour->input = NULL;
  behaviour->inputname = NULL;
}

static int validatebehaviour(KlCBehaviour* behaviour) {
  if (!(behaviour->option & OPTION_HELP) && !behaviour->input) {
    if (kl_unlikely(!(behaviour->input = kifile_attach_keepcontent(stdin)))) {
      cleanbehaviour(behaviour);
      return 1;
    }
    behaviour->option |= OPTION_STDIN;
    behaviour->inputname = "stdin";
  }
  return 0;
}

static void print_help(void) {
  printf("Usage: klangc [<inputfile>] [options [<filename>]]\n");
  printf("options:\n");
  printf("  -h --help                   show this message.\n");
  printf("  -d --dump <filename>        dump the bytecode to file.\n");
  printf("  -u --undump                 take the inputfile as a bytecode file.\n");
  printf("  -p --print [<filename>]     print the bytecode to <filename>(stdout\n");
  printf("                              if not specified).\n");
  printf("if <inputfile> is not specified, read from stdin.\n");
}
