#include "include/ast/klstrtbl.h"
#include "include/os_spec/kfs.h"
#include "include/parse/kllex.h"
#include "include/parse/klparser.h"
#include "include/code/klcode.h"
#include "deps/k/include/kio/kifile.h"
#include "deps/k/include/kio/kofile.h"
#include "deps/k/include/kio/kibuf.h"
#include <stdarg.h>

#define KLC_OPTION_NORMAL   ((unsigned)0)
#define KLC_OPTION_HELP     ((unsigned)klbit(0))
#define KLC_OPTION_PRINT    ((unsigned)klbit(1))
#define KLC_OPTION_DUMP     ((unsigned)klbit(2))
#define KLC_OPTION_UNDUMP   ((unsigned)klbit(3))
#define KLC_OPTION_STDIN    ((unsigned)klbit(4))


#define KLC_NARGRAW(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, ...)     (_9)
#define KLC_NARG(...)         KLC_NARGRAW(__VA_ARGS__ , 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)
#define klc_match(opt, ...)   klc_matchraw((opt), KLC_NARG(__VA_ARGS__), __VA_ARGS__)
#define klc_isfilename(arg)   ((arg)[0] != '-')

typedef struct tagKlCBehaviour {
  Ki* input;
  const char* inputname;
  Ko* dumpoutput;
  Ko* textoutput;
  unsigned option;
} KlCBehaviour;

static int klc_parse_argv(int argc, char** argv, KlCBehaviour* behaviour);
static KlCode* klc_undump(KlCBehaviour* behaviour, KlStrTbl* strtbl);
static KlCode* klc_compile(KlCBehaviour* behaviour, KlStrTbl* strtbl, KlError* klerr);
static KlCode* klc_getcode(KlCBehaviour* behaviour, KlStrTbl* strtbl, KlError* klerr);
static int klc_output(KlCBehaviour* behaviour, KlCode* code);
static int klc_do(KlCBehaviour* behaviour);
static bool klc_matchraw(const char* arg, unsigned nopts, ...);
static void klc_cleanbehaviour(KlCBehaviour* behaviour);
static int klc_validatebehaviour(KlCBehaviour* behaviour);
static void klc_print_help(void);



int main(int argc, char** argv) {
  KlCBehaviour behaviour;
  int ret = klc_parse_argv(argc, argv, &behaviour);
  if (kl_unlikely(ret)) return ret;
  return klc_do(&behaviour);
}

static int klc_parse_argv(int argc, char** argv, KlCBehaviour* behaviour) {
  behaviour->input = NULL;
  behaviour->inputname = NULL;
  behaviour->dumpoutput = NULL;
  behaviour->textoutput = NULL;
  behaviour->option = KLC_OPTION_NORMAL;
  if (argc == 1) {
    fprintf(stderr, "no argument\n");
    klc_print_help();
    klc_cleanbehaviour(behaviour);
    return 1;
  }
  for (int i = 1; i < argc; ++i) {
    if (klc_match(argv[i], "-h", "--help")) {
      behaviour->option |= KLC_OPTION_HELP;
    } else if (klc_match(argv[i], "-p", "--print")) {
      behaviour->option |= KLC_OPTION_PRINT;
      if (behaviour->textoutput)
        ko_delete(behaviour->textoutput);
      bool isfilename = argv[i + 1] && klc_isfilename(argv[i + 1]);
      behaviour->textoutput = isfilename
        ? kofile_create(argv[i + 1])
        : kofile_attach(stdout);
      if (isfilename) ++i;
      if (kl_unlikely(!behaviour->textoutput)) {
        if (isfilename) {
          fprintf(stderr, "failed to open file: %s\n", argv[i]);
        } else {
          fprintf(stderr, "failed to attach to stdout\n");
        }
        klc_cleanbehaviour(behaviour);
        return 1;
      }
    } else if (klc_match(argv[i], "-d", "--dump")) {
      behaviour->option |= KLC_OPTION_DUMP;
      if (!argv[i + 1] || !klc_isfilename(argv[i + 1])) {
        fprintf(stderr, "%s should specify output file\n", argv[i]);
        klc_cleanbehaviour(behaviour);
        return 1;
      }
      if (behaviour->dumpoutput)
        ko_delete(behaviour->dumpoutput);
      behaviour->dumpoutput = kofile_create(argv[++i]);
      if (kl_unlikely(!behaviour->dumpoutput)) {
        fprintf(stderr, "failed to open file: %s\n", argv[i]);
        klc_cleanbehaviour(behaviour);
        return 1;
      }
    } else if (klc_match(argv[i], "-u", "--undump")) {
      behaviour->option |= KLC_OPTION_UNDUMP;
    } else if (klc_isfilename(argv[i])) {
      if (behaviour->input) {
        fprintf(stderr, "the input is specified more than once\n");
        klc_cleanbehaviour(behaviour);
        return 1;
      }
      if (kl_unlikely(!(behaviour->input = kifile_create(argv[i])))) {
        fprintf(stderr, "failed to open file: %s\n", argv[i]);
        klc_cleanbehaviour(behaviour);
        return 1;
      }
      behaviour->inputname = argv[i];
    } else {
      fprintf(stderr, "unrecognized option: %s\n", argv[i]);
      klc_print_help();
      klc_cleanbehaviour(behaviour);
      return 1;
    }
  }
  return klc_validatebehaviour(behaviour);
}

static KlCode* klc_undump(KlCBehaviour* behaviour, KlStrTbl* strtbl) {
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

static int klc_getsrcfilename(KlCBehaviour* behaviour, KlStrTbl* strtbl, KlStrDesc* srcfile) {
  if (behaviour->option & KLC_OPTION_STDIN) {
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

static KlCode* klc_compile(KlCBehaviour* behaviour, KlStrTbl* strtbl, KlError* klerr) {
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
  if (kl_unlikely(klc_getsrcfilename(behaviour, strtbl, &srcfilename))) {
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

static KlCode* klc_getcode(KlCBehaviour* behaviour, KlStrTbl* strtbl, KlError* klerr) {
  return behaviour->option & KLC_OPTION_UNDUMP ? klc_undump(behaviour, strtbl)
                                               : klc_compile(behaviour, strtbl, klerr);
  
}

static int klc_output(KlCBehaviour* behaviour, KlCode* code) {
  if (behaviour->option & KLC_OPTION_PRINT)
    klcode_print(code, behaviour->textoutput);
  if (behaviour->option & KLC_OPTION_DUMP) {
    if (kl_unlikely(!klcode_dump(code, behaviour->dumpoutput))) {
      fprintf(stderr, "dump failure\n");
      return 1;
    }
  }
  return 0;
}

static int klc_do(KlCBehaviour* behaviour) {
  if (behaviour->option & KLC_OPTION_HELP) {
    klc_print_help();
    klc_cleanbehaviour(behaviour);
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
  KlCode* code = klc_getcode(behaviour, strtbl, &klerr);
  if (kl_unlikely(!code)) {
    ret = 1;
    goto getcode_error;
  }

  ret = klc_output(behaviour, code);
  klcode_delete(code);
getcode_error:
  ko_delete(errout);
  klstrtbl_delete(strtbl);
  klc_cleanbehaviour(behaviour);
  return ret;

error_errout:
  klstrtbl_delete(strtbl);
error_strtbl:
  fprintf(stderr, "fatal error: out of memory\n");
  klc_cleanbehaviour(behaviour);
  return 1;
}

static bool klc_matchraw(const char* arg, unsigned nopts, ...) {
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

static void klc_cleanbehaviour(KlCBehaviour* behaviour) {
  if (behaviour->textoutput) ko_delete(behaviour->textoutput);
  if (behaviour->dumpoutput) ko_delete(behaviour->dumpoutput);
  if (behaviour->input) ki_delete(behaviour->input);
  behaviour->textoutput = NULL;
  behaviour->dumpoutput = NULL;
  behaviour->input = NULL;
  behaviour->inputname = NULL;
}

static int klc_validatebehaviour(KlCBehaviour* behaviour) {
  if (!(behaviour->option & KLC_OPTION_HELP) && !behaviour->input) {
    if (kl_unlikely(!(behaviour->input = kifile_attach_keepcontent(stdin)))) {
      klc_cleanbehaviour(behaviour);
      return 1;
    }
    behaviour->option |= KLC_OPTION_STDIN;
    behaviour->inputname = "stdin";
  }
  return 0;
}

static void klc_print_help(void) {
  printf("Usage: klangc [<inputfile>] [options [<filename>]]\n");
  printf("options:\n");
  printf("  -h --help                   show this message.\n");
  printf("  -d --dump <filename>        dump the bytecode to file.\n");
  printf("  -u --undump                 take the inputfile as a bytecode file.\n");
  printf("  -p --print [<filename>]     print the bytecode to <filename>(stdout\n");
  printf("                              if not specified).\n");
  printf("if <inputfile> is not specified, read from stdin.\n");
}
