#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "lexgen/include/lexgen/cmdline.h"
#include "utils/include/dir.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void kev_lexgen_set_value(char* arg, KevOptions* options);
static void kev_lexgen_set_default(KevOptions* options);
static void kev_lexgen_set_out_path(char* out, KevOptions* options);
static void kev_lexgen_set_build_tool(KevOptions* options);
static void error(char* info, char* info2);
static char* copy_string(char* str);
static char* kev_get_value(char* arg, char* prefix);
static void kev_add_lang_postfix(char* str, char* language);

void kev_lexgen_get_options(int argc, char** argv, KevOptions* options) {
  char* out = NULL; /* temperarily store the output path */
  kev_lexgen_set_default(options);
  for (size_t i = 1; i < argc; ++i) {
    char* arg = argv[i];
    if (strcmp(arg, "-o") == 0 || strcmp(arg, "--out") == 0) {
      if (++i >= argc)
        error("missing output path: ", "-o <path>");
      out = copy_string(argv[i]);
    } else if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0) {
      options->opts[KEV_LEXGEN_OPT_HELP] = KEV_LEXGEN_OPT_TRUE;
    } else if (strcmp(arg, "-i") == 0 || strcmp(arg, "--in") == 0) {
      if (++i >= argc)
        error("missing input path: ", "-i <path>");
      free(options->strs[KEV_LEXGEN_INPUT_PATH]);
      options->strs[KEV_LEXGEN_INPUT_PATH] = copy_string(argv[i]);
    } else if (strcmp(arg, "--out-inc") == 0) {
      if (++i >= argc)
        error("missing output path: ", "-out-inc <path>");
      free(options->strs[KEV_LEXGEN_OUT_INC_PATH]);
      options->strs[KEV_LEXGEN_OUT_INC_PATH] = copy_string(argv[i]);
    } else if (strcmp(arg, "--out-info") == 0) {
      options->opts[KEV_LEXGEN_OPT_PUT_INFO] = KEV_LEXGEN_OPT_TRUE;
    } else if (strcmp(arg, "--out-callback") == 0) {
      options->opts[KEV_LEXGEN_OPT_PUT_CALLBACK] = KEV_LEXGEN_OPT_TRUE;
    } else {
      kev_lexgen_set_value(arg, options);
    }
  }
  if (options->opts[KEV_LEXGEN_OPT_HELP]) return;
  if (!options->strs[KEV_LEXGEN_INPUT_PATH]) {
    error("input file is not specified", NULL);
  }
  if (!out) {
    error("output path is not specified", NULL);
  }
  kev_lexgen_set_out_path(out, options);
  kev_lexgen_set_build_tool(options);
}

static void error(char* info, char* info2) {
  if (info)
    fprintf(stderr, "%s", info);
  if (info2)
    fprintf(stderr, "%s\n", info2);
  exit(EXIT_FAILURE);
}

static char* copy_string(char* str) {
  char* ret = (char*)malloc(sizeof (char) * (strlen(str) + 1));
  if (!ret) error("out of memory", NULL);
  strcpy(ret, str);
  return ret;
}

static void kev_lexgen_set_value(char* arg, KevOptions* options) {
  if (arg[0] != '-')
    error("not an option: ", arg);
  char* value = NULL;
  if ((value = kev_get_value(arg, "-s=")) || (value = kev_get_value(arg, "--stage="))) {
    if (strcmp(value, "t") == 0 || strcmp(value, "trans") == 0 ||
        strcmp(value, "transition") == 0) {
      options->opts[KEV_LEXGEN_OPT_STAGE] = KEV_LEXGEN_OPT_STA_TAB;
    } else if (strcmp(value, "s") == 0 || strcmp(value, "source") == 0) {
      options->opts[KEV_LEXGEN_OPT_STAGE] = KEV_LEXGEN_OPT_STA_SRC;
    } else if (strcmp(value, "a") == 0 || strcmp(value, "archive") == 0) {
      options->opts[KEV_LEXGEN_OPT_STAGE] = KEV_LEXGEN_OPT_STA_ARC;
    } else if (strcmp(value, "sh") == 0 || strcmp(value, "shared") == 0) {
      options->opts[KEV_LEXGEN_OPT_STAGE] = KEV_LEXGEN_OPT_STA_SHA;
    } else {
      error("unknown --stage value: ", value);
    }
  } else if ((value = kev_get_value(arg, "-l=")) || (value = kev_get_value(arg, "--lang=")) ||
             (value = kev_get_value(arg, "--language="))) {
    free(options->strs[KEV_LEXGEN_LANG_NAME]);
    if (strcmp(value, "c") == 0) {
      options->strs[KEV_LEXGEN_LANG_NAME] = copy_string(value);
    } else if (strcmp(value, "cpp") == 0 || strcmp(value, "cxx") == 0 ||
               strcmp(value, "cc") == 0 || strcmp(value, "cx") == 0 ||
               strcmp(value, "c++") == 0) {
      options->strs[KEV_LEXGEN_LANG_NAME] = copy_string("cpp");
    } else if (strcmp(value, "ru") == 0 || strcmp(value, "rust") == 0) {
      options->strs[KEV_LEXGEN_LANG_NAME] = copy_string("rust");
    } else {
      error("unsupported language: ", value);
    }
  } else if ((value = kev_get_value(arg, "--build-tool="))) {
    free(options->strs[KEV_LEXGEN_BUILD_TOOL_NAME]);
    options->strs[KEV_LEXGEN_BUILD_TOOL_NAME] = copy_string(value);
  } else if ((value = kev_get_value(arg, "--length="))) {
    if (strcmp(value, "8") == 0)
      options->opts[KEV_LEXGEN_OPT_WIDTH] = 8;
    else if (strcmp(value, "16") == 0)
      options->opts[KEV_LEXGEN_OPT_WIDTH] = 16;
    else
      error("--length can only be 8 or 16", NULL);
  } else if ((value = kev_get_value(arg, "charset"))) {
    if (strcmp(value, "utf-8") == 0)
      options->opts[KEV_LEXGEN_OPT_CHARSET] = KEV_LEXGEN_OPT_CHARSET_UTF8;
    else if (strcmp(value, "ascii") == 0)
      options->opts[KEV_LEXGEN_OPT_CHARSET] = KEV_LEXGEN_OPT_CHARSET_ASCII;
    else
      error("--charset can only be utf-8 or ascii", NULL);
  } else {
    error("unknown option: ", arg);
  }
}

static void kev_lexgen_set_default(KevOptions* options) {
  for (size_t i = 0; i < KEV_LEXGEN_STR_NO; ++i)
    options->strs[i] = NULL;
  options->opts[KEV_LEXGEN_OPT_PUT_CALLBACK] = KEV_LEXGEN_OPT_FALSE;
  options->opts[KEV_LEXGEN_OPT_HELP] = KEV_LEXGEN_OPT_FALSE;
  options->opts[KEV_LEXGEN_OPT_PUT_INFO] = KEV_LEXGEN_OPT_FALSE;
  options->opts[KEV_LEXGEN_OPT_CHARSET] = KEV_LEXGEN_OPT_CHARSET_UTF8;
  options->opts[KEV_LEXGEN_OPT_WIDTH] = 8;
  options->opts[KEV_LEXGEN_OPT_STAGE] = KEV_LEXGEN_OPT_STA_TAB;
  options->strs[KEV_LEXGEN_LANG_NAME] = copy_string("c");
}

static void kev_lexgen_set_out_path(char* out, KevOptions* options) {
  if (options->opts[KEV_LEXGEN_OPT_STAGE] == KEV_LEXGEN_OPT_STA_TAB) {
    options->strs[KEV_LEXGEN_OUT_TAB_PATH] = out;
  } else if (options->opts[KEV_LEXGEN_OPT_STAGE] == KEV_LEXGEN_OPT_STA_SRC) {
    options->strs[KEV_LEXGEN_OUT_TAB_PATH] = out;
    options->opts[KEV_LEXGEN_OPT_PUT_INFO] = KEV_LEXGEN_OPT_TRUE;
    options->opts[KEV_LEXGEN_OPT_PUT_CALLBACK] = KEV_LEXGEN_OPT_TRUE;
  } else if (options->opts[KEV_LEXGEN_OPT_STAGE] == KEV_LEXGEN_OPT_STA_ARC ||
             options->opts[KEV_LEXGEN_OPT_STAGE] == KEV_LEXGEN_OPT_STA_SHA) {
    options->opts[KEV_LEXGEN_OPT_PUT_INFO] = KEV_LEXGEN_OPT_TRUE;
    options->opts[KEV_LEXGEN_OPT_PUT_CALLBACK] = KEV_LEXGEN_OPT_TRUE;
    free(options->strs[KEV_LEXGEN_OUT_INC_PATH]);
    free(options->strs[KEV_LEXGEN_OUT_SRC_PATH]);
    char* tmp_path = kev_get_lexgen_tmp_dir();
    char* out_path = (char*)malloc(sizeof (char) * (strlen(tmp_path) + 17));
    if (!out_path) error("out of memory", NULL);
    strcpy(out_path, tmp_path);
    strcat(out_path, "table");
    kev_add_lang_postfix(out_path, options->strs[KEV_LEXGEN_LANG_NAME]);
    options->strs[KEV_LEXGEN_OUT_TAB_PATH] = out_path;
    out_path = (char*)malloc(sizeof (char) * (strlen(tmp_path) + 17));
    if (!out_path) error("out of memory", NULL);
    strcpy(out_path, tmp_path);
    strcat(out_path, "lexer");
    kev_add_lang_postfix(out_path, options->strs[KEV_LEXGEN_LANG_NAME]);
    options->strs[KEV_LEXGEN_OUT_SRC_PATH] = out_path;
    if (strcmp(options->strs[KEV_LEXGEN_LANG_NAME], "c") == 0 ||
        strcmp(options->strs[KEV_LEXGEN_LANG_NAME], "cpp") == 0) {
      out_path = (char*)malloc(sizeof (char) * (strlen(tmp_path) + 17));
      if (!out_path) error("out of memory", NULL);
      strcpy(out_path, tmp_path);
      strcat(out_path, "lexer.h");
    }
  }
}

static void kev_lexgen_set_build_tool(KevOptions* options) {
  if ( options->strs[KEV_LEXGEN_BUILD_TOOL_NAME]) return;
  if (strcmp(options->strs[KEV_LEXGEN_LANG_NAME], "rust") == 0)
    error("rust currently not supported", NULL);  /* TODO: support rust */
  else if (strcmp(options->strs[KEV_LEXGEN_LANG_NAME], "cpp") == 0)
    options->strs[KEV_LEXGEN_BUILD_TOOL_NAME] = copy_string("g++");
  else if (strcmp(options->strs[KEV_LEXGEN_LANG_NAME], "c") == 0)
    options->strs[KEV_LEXGEN_BUILD_TOOL_NAME] = copy_string("gcc");
  else
    error("not supported language: ", options->strs[KEV_LEXGEN_LANG_NAME]);
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

static void kev_add_lang_postfix(char* str, char* language) {
  if (strcmp(language, "rust") == 0)
    strcat(str, ".ru");
  else if (strcmp(language, "c") == 0)
    strcat(str, ".c");
  else if (strcmp(str, "cpp") == 0)
    strcat(str, ".cpp");
  else
    error("internal error in kev_add_lang_postfix(), wrong language name: ", language);
}
