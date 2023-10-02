#include "lexgen/include/lexgen/cmdline.h"
#include "lexgen/include/lexgen/dir.h"
#include "lexgen/include/lexgen/error.h"
#include "utils/include/string/kev_string.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* handle kev-value pair options */
static void kev_lexgen_set_kv_pair(char* arg, KevLOptions* options);
/* set value for options before resolve argv */
static void kev_lexgen_set_pre(KevLOptions* options);
/* set value for options after resolve argv */
static void kev_lexgen_set_post(KevLOptions* options);
/* get a copy of 'str' */
static char* copy_string(const char* str);
/* get value in a key-value pair */
static const char* kev_get_value(const char* arg, const char* prefix);

void kev_lexgen_get_options(int argc, char** argv, KevLOptions* options) {
  kev_lexgen_set_pre(options);
  for (int i = 1; i < argc; ++i) {
    char* arg = argv[i];
    if (strcmp(arg, "-o") == 0 || strcmp(arg, "--out") == 0) {
      if (++i >= argc)
        kev_throw_error("command line parser:", "missing output path: ", "-o <path>");
      free(options->strs[KEV_LEXGEN_OUT_SRC_PATH]);
      options->strs[KEV_LEXGEN_OUT_SRC_PATH] = copy_string(argv[i]);
    } else if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0) {
      options->opts[KEV_LEXGEN_OPT_HELP] = KEV_LEXGEN_OPT_TRUE;
    } else if (strcmp(arg, "-i") == 0 || strcmp(arg, "--in") == 0) {
      if (++i >= argc)
        kev_throw_error("command line parser:", "missing input path: ", "-i <path>");
      free(options->strs[KEV_LEXGEN_INPUT_PATH]);
      options->strs[KEV_LEXGEN_INPUT_PATH] = copy_string(argv[i]);
    } else if (strcmp(arg, "--out-inc") == 0) {
      if (++i >= argc)
        kev_throw_error("command line parser:", "missing output path: ", "--out-inc <path>");
      free(options->strs[KEV_LEXGEN_OUT_INC_PATH]);
      options->strs[KEV_LEXGEN_OUT_INC_PATH] = copy_string(argv[i]);
  } else if (strcmp(arg, "-t") == 0 || strcmp(arg, "--tmpl") == 0) {
      if (++i >= argc)
        kev_throw_error("command line parser:", "missing template path: ", "--template <path>");
    free(options->strs[KEV_LEXGEN_SRC_TMPL_PATH]);
    options->strs[KEV_LEXGEN_SRC_TMPL_PATH] = copy_string(argv[i]);
  } else if (strcmp(arg, "--it") == 0 || strcmp(arg, "--inc-tmpl") == 0) {
      if (++i >= argc)
        kev_throw_error("command line parser:", "missing header template path: ", "--inc-template <path>");
    free(options->strs[KEV_LEXGEN_INC_TMPL_PATH]);
    options->strs[KEV_LEXGEN_INC_TMPL_PATH] = copy_string(argv[i]);
    } else {
      kev_lexgen_set_kv_pair(arg, options);
    }
  }
  if (options->opts[KEV_LEXGEN_OPT_HELP]) return;
  if (!options->strs[KEV_LEXGEN_INPUT_PATH]) {
    kev_throw_error("command line parser:", "input file is not specified", NULL);
  }
  kev_lexgen_set_post(options);
}

static char* copy_string(const char* str) {
  char* ret = (char*)malloc(sizeof (char) * (strlen(str) + 1));
  if (!ret) kev_throw_error("command line parser:", "out of memory", NULL);
  strcpy(ret, str);
  return ret;
}

static void kev_lexgen_set_kv_pair(char* arg, KevLOptions* options) {
  if (arg[0] != '-')
    kev_throw_error("command line parser:", "not an option: ", arg);
  const char* value = NULL;
  if ((value = kev_get_value(arg, "-l=")) || (value = kev_get_value(arg, "--lang=")) ||
      (value = kev_get_value(arg, "--language="))) {
    free(options->strs[KEV_LEXGEN_LANG_NAME]);
    options->strs[KEV_LEXGEN_LANG_NAME] = copy_string(value);
  } else {
    kev_throw_error("command line parser", "unknown option: ", arg);
  }
}

static void kev_lexgen_set_pre(KevLOptions* options) {
  for (size_t i = 0; i < KEV_LEXGEN_STR_NO; ++i)
    options->strs[i] = NULL;
  options->opts[KEV_LEXGEN_OPT_HELP] = KEV_LEXGEN_OPT_FALSE;
  options->strs[KEV_LEXGEN_LANG_NAME] = copy_string("c");
  options->strs[KEV_LEXGEN_OUT_SRC_PATH] = copy_string("lex.out");
}

static void kev_lexgen_set_post(KevLOptions* options) {
  char* resources_dir = kev_get_lexgen_resources_dir();
  if (!resources_dir)
    kev_throw_error("output:", "can not get resources directory", NULL);
  if (!options->strs[KEV_LEXGEN_SRC_TMPL_PATH]) {
    char* src_path = kev_str_concat(resources_dir, "lexer/");
    char* tmp = src_path;
    src_path = kev_str_concat(tmp, options->strs[KEV_LEXGEN_LANG_NAME]);
    free(tmp);
    tmp = src_path;
    src_path = kev_str_concat(tmp, "/src.tmpl");
    free(tmp);
    if (!src_path)
      kev_throw_error("output:", "out of memory", NULL);
    options->strs[KEV_LEXGEN_SRC_TMPL_PATH] = src_path;
  }

  if (!options->strs[KEV_LEXGEN_INC_TMPL_PATH]) {
    char* inc_path = kev_str_concat(resources_dir, "lexer/");
    char* tmp = inc_path;
    inc_path = kev_str_concat(tmp, options->strs[KEV_LEXGEN_LANG_NAME]);
    free(tmp);
    tmp = inc_path;
    inc_path = kev_str_concat(tmp, "/inc.tmpl");
    free(tmp);
    if (!inc_path)
      kev_throw_error("output:", "out of memory", NULL);
    options->strs[KEV_LEXGEN_INC_TMPL_PATH] = inc_path;
  }
  free(resources_dir);
}

void kev_lexgen_destroy_options(KevLOptions* options) {
  for (size_t i = 0; i < KEV_LEXGEN_STR_NO; ++i)
    free(options->strs[i]);
}

static const char* kev_get_value(const char* arg, const char* prefix) {
  size_t len = strlen(prefix);
  if (strncmp(arg, prefix, len) == 0)
    return arg + len;
  else
    return NULL;
}
