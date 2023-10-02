#include "pargen/include/pargen/cmdline.h"
#include "pargen/include/pargen/dir.h"
#include "pargen/include/pargen/error.h"
#include "utils/include/string/kev_string.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* handle kev-value pair options */
static void kev_pargen_set_kv_pair(char* arg, KevPOptions* options);
/* set value for options before resolve argv */
static void kev_pargen_set_pre(KevPOptions* options);
/* set value for options after resolve argv */
static void kev_pargen_set_post(KevPOptions* options);
/* get a copy of 'str' */
static char* copy_string(const char* str);
/* get option value from a key-value pair */
static const char* kev_get_value(const char* arg, const char* prefix);

static void kev_pargen_set_all_info_path(KevPOptions* options, const char* output_dir);

void kev_pargen_get_options(int argc, char** argv, KevPOptions* options) {
  kev_pargen_set_pre(options);
  for (int i = 1; i < argc; ++i) {
    char* arg = argv[i];
    if (strcmp(arg, "-o") == 0 || strcmp(arg, "--out") == 0) {
      if (++i >= argc)
        kev_throw_error("command line parser:", "missing output path: ", "-o <path>");
      free(options->strs[KEV_PARGEN_OUT_SRC_PATH]);
      options->strs[KEV_PARGEN_OUT_SRC_PATH] = copy_string(argv[i]);
    } else if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0) {
      options->opts[KEV_PARGEN_OPT_HELP] = KEV_PARGEN_OPT_TRUE;
    } else if (strcmp(arg, "-i") == 0 || strcmp(arg, "--in") == 0) {
      if (++i >= argc)
        kev_throw_error("command line parser:", "missing input path: ", "-i <path>");
      free(options->strs[KEV_PARGEN_INPUT_PATH]);
      options->strs[KEV_PARGEN_INPUT_PATH] = copy_string(argv[i]);
    } else if (strcmp(arg, "--out-inc") == 0) {
      if (++i >= argc)
        kev_throw_error("command line parser:", "missing output path: ", "--out-inc <path>");
      free(options->strs[KEV_PARGEN_OUT_INC_PATH]);
      options->strs[KEV_PARGEN_OUT_INC_PATH] = copy_string(argv[i]);
    } else if (strcmp(arg, "-t") == 0 || strcmp(arg, "--tmpl") == 0) {
      if (++i >= argc)
        kev_throw_error("command line parser:", "missing template file path: ", "--tmpl <path>");
      free(options->strs[KEV_PARGEN_SRC_TMPL_PATH]);
      options->strs[KEV_PARGEN_SRC_TMPL_PATH] = copy_string(argv[i]);
    } else if (strcmp(arg, "--it") == 0 || strcmp(arg, "--inc-tmpl") == 0) {
      if (++i >= argc)
        kev_throw_error("command line parser:", "missing header template file path: ", "--inc-tmpl <path>");
      free(options->strs[KEV_PARGEN_INC_TMPL_PATH]);
      options->strs[KEV_PARGEN_INC_TMPL_PATH] = copy_string(argv[i]);
    } else if (strcmp(arg, "--li") == 0 || strcmp(arg, "--lrinfo") == 0) {
      const char* output_dir = "";
      if (i + 1 < argc && argv[i + 1][0] != '-') {
        output_dir = argv[++i];
      }
      kev_pargen_set_all_info_path(options, output_dir);
    } else {
      kev_pargen_set_kv_pair(arg, options);
    }
  }
  if (options->opts[KEV_PARGEN_OPT_HELP]) return;
  if (!options->strs[KEV_PARGEN_INPUT_PATH]) {
    kev_throw_error("command line parser:", "input file is not specified", NULL);
  }
  kev_pargen_set_post(options);
}

static char* copy_string(const char* str) {
  char* ret = (char*)malloc(sizeof (char) * (strlen(str) + 1));
  if (!ret) kev_throw_error("command line parser:", "out of memory", NULL);
  strcpy(ret, str);
  return ret;
}

static void kev_pargen_set_kv_pair(char* arg, KevPOptions* options) {
  if (arg[0] != '-')
    kev_throw_error("command line parser:", "not an option: ", arg);
  const char* value = NULL;
  if ((value = kev_get_value(arg, "-l=")) || (value = kev_get_value(arg, "--lang=")) ||
      (value = kev_get_value(arg, "--language="))) {
    free(options->strs[KEV_PARGEN_LANG_NAME]);
    options->strs[KEV_PARGEN_LANG_NAME] = copy_string(value);
  } else {
    kev_throw_error("command line parser", "unknown option: ", arg);
  }
}

static void kev_pargen_set_pre(KevPOptions* options) {
  for (size_t i = 0; i < KEV_PARGEN_STR_NO; ++i)
    options->strs[i] = NULL;
  options->opts[KEV_PARGEN_OPT_HELP] = KEV_PARGEN_OPT_FALSE;
  options->strs[KEV_PARGEN_LANG_NAME] = copy_string("c");
  options->strs[KEV_PARGEN_OUT_SRC_PATH] = copy_string("parser.out");
}

static void kev_pargen_set_post(KevPOptions* options) {
  char* resources_dir = kev_get_pargen_resources_dir();
  if (!resources_dir)
    kev_throw_error("command line parser:", "can not get resources directory", NULL);
  if (!options->strs[KEV_PARGEN_SRC_TMPL_PATH]) {
    char* src_path = kev_str_concat(resources_dir, "parser/");
    char* tmp = src_path;
    src_path = kev_str_concat(tmp, options->strs[KEV_PARGEN_LANG_NAME]);
    free(tmp);
    tmp = src_path;
    src_path = kev_str_concat(tmp, "/src.tmpl");
    free(tmp);
    if (!src_path)
      kev_throw_error("command line parser:", "out of memory", NULL);
    options->strs[KEV_PARGEN_SRC_TMPL_PATH] = src_path;
  }

  if (!options->strs[KEV_PARGEN_INC_TMPL_PATH]) {
    char* inc_path = kev_str_concat(resources_dir, "parser/");
    char* tmp = inc_path;
    inc_path = kev_str_concat(tmp, options->strs[KEV_PARGEN_LANG_NAME]);
    free(tmp);
    tmp = inc_path;
    inc_path = kev_str_concat(tmp, "/inc.tmpl");
    free(tmp);
    if (!inc_path)
      kev_throw_error("command line parser:", "out of memory", NULL);
    options->strs[KEV_PARGEN_INC_TMPL_PATH] = inc_path;
  }
}

void kev_pargen_destroy_options(KevPOptions* options) {
  for (size_t i = 0; i < KEV_PARGEN_STR_NO; ++i)
    free(options->strs[i]);
}

static const char* kev_get_value(const char* arg, const char* prefix) {
  size_t len = strlen(prefix);
  if (strncmp(arg, prefix, len) == 0)
    return arg + len;
  else
    return NULL;
}

static void kev_pargen_set_all_info_path(KevPOptions* options, const char* output_dir) {
  if (!(options->strs[KEV_PARGEN_LRINFO_GOTO_PATH] = kev_str_concat(output_dir, "goto.txt")))
    kev_throw_error("command line parser:", "out of memory", NULL);
  if (!(options->strs[KEV_PARGEN_LRINFO_ACTION_PATH] = kev_str_concat(output_dir, "action.txt")))
    kev_throw_error("command line parser:", "out of memory", NULL);
  if (!(options->strs[KEV_PARGEN_LRINFO_COLLEC_PATH] = kev_str_concat(output_dir, "collection.txt")))
    kev_throw_error("command line parser:", "out of memory", NULL);
  if (!(options->strs[KEV_PARGEN_LRINFO_SYMBOL_PATH] = kev_str_concat(output_dir, "symbol.txt")))
    kev_throw_error("command line parser:", "out of memory", NULL);
}
