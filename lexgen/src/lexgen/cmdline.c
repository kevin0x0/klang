#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "lexgen/include/lexgen/cmdline.h"
#include "utils/include/dir.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* handle kev-value pair options */
static void kev_lexgen_set_kv_pair(char* arg, KevOptions* options);
/* set default value for options */
static void kev_lexgen_set_default(KevOptions* options);
static void error(char* info, char* info2);
/* get a copy of 'str' */
static char* copy_string(char* str);
/* get value in a key-value pair */
static char* kev_get_value(char* arg, char* prefix);

void kev_lexgen_get_options(int argc, char** argv, KevOptions* options) {
  char* out = NULL; /* temperarily store the output path */
  kev_lexgen_set_default(options);
  for (size_t i = 1; i < argc; ++i) {
    char* arg = argv[i];
    if (strcmp(arg, "-o") == 0 || strcmp(arg, "--out") == 0) {
      if (++i >= argc)
        error("missing output path: ", "-o <path>");
      free(options->strs[KEV_LEXGEN_OUT_SRC_PATH]);
      options->strs[KEV_LEXGEN_OUT_SRC_PATH] = copy_string(argv[i]);
    } else if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0) {
      options->opts[KEV_LEXGEN_OPT_HELP] = KEV_LEXGEN_OPT_TRUE;
    } else if (strcmp(arg, "-i") == 0 || strcmp(arg, "--in") == 0) {
      if (++i >= argc)
        error("missing input path: ", "-i <path>");
      free(options->strs[KEV_LEXGEN_INPUT_PATH]);
      options->strs[KEV_LEXGEN_INPUT_PATH] = copy_string(argv[i]);
    } else if (strcmp(arg, "--out-inc") == 0) {
      if (++i >= argc)
        error("missing output path: ", "--out-inc <path>");
      free(options->strs[KEV_LEXGEN_OUT_INC_PATH]);
      options->strs[KEV_LEXGEN_OUT_INC_PATH] = copy_string(argv[i]);
    } else {
      kev_lexgen_set_kv_pair(arg, options);
    }
  }
  if (options->opts[KEV_LEXGEN_OPT_HELP]) return;
  if (!options->strs[KEV_LEXGEN_INPUT_PATH]) {
    error("input file is not specified", NULL);
  }
  if (!out) {
    error("output path is not specified", NULL);
  }
}

static void error(char* info, char* info2) {
  if (info)
    fprintf(stderr, "%s", info);
  if (info2)
    fprintf(stderr, "%s", info2);
  fputc('\n', stderr);
  exit(EXIT_FAILURE);
}

static char* copy_string(char* str) {
  char* ret = (char*)malloc(sizeof (char) * (strlen(str) + 1));
  if (!ret) error("out of memory", NULL);
  strcpy(ret, str);
  return ret;
}

static void kev_lexgen_set_kv_pair(char* arg, KevOptions* options) {
  if (arg[0] != '-')
    error("not an option: ", arg);
  char* value = NULL;
  if ((value = kev_get_value(arg, "-l=")) || (value = kev_get_value(arg, "--lang=")) ||
             (value = kev_get_value(arg, "--language="))) {
    free(options->strs[KEV_LEXGEN_LANG_NAME]);
    if (strcmp(value, "c") == 0) {
      options->strs[KEV_LEXGEN_LANG_NAME] = copy_string(value);
    } else if (strcmp(value, "cpp") == 0 || strcmp(value, "cxx") == 0 ||
               strcmp(value, "cc") == 0 || strcmp(value, "cx") == 0 ||
               strcmp(value, "c++") == 0) {
      options->strs[KEV_LEXGEN_LANG_NAME] = copy_string("cpp");
    } else if (strcmp(value, "rs") == 0 || strcmp(value, "rust") == 0) {
      options->strs[KEV_LEXGEN_LANG_NAME] = copy_string("rust");
    } else {
      error("unsupported language: ", value);
    }
  } else {
    error("unknown option: ", arg);
  }
}

static void kev_lexgen_set_default(KevOptions* options) {
  for (size_t i = 0; i < KEV_LEXGEN_STR_NO; ++i)
    options->strs[i] = NULL;
  options->opts[KEV_LEXGEN_OPT_HELP] = KEV_LEXGEN_OPT_FALSE;
  options->opts[KEV_LEXGEN_OPT_TAB_ONLY] = KEV_LEXGEN_OPT_FALSE;
  options->strs[KEV_LEXGEN_LANG_NAME] = copy_string("c");
}

void kev_lexgen_destroy_options(KevOptions* options) {
  for (size_t i = 0; i < KEV_LEXGEN_STR_NO; ++i)
    free(options->strs[i]);
}

static char* kev_get_value(char* arg, char* prefix) {
  size_t len = strlen(prefix);
  if (strncmp(arg, prefix, len) == 0)
    return arg + len;
  else
    return NULL;
}
