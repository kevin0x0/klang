#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "lexgen/include/lexgen/control.h"
#include "lexgen/include/lexgen/convert.h"
#include "lexgen/include/lexgen/output.h"
#include "utils/include/dir.h"

#include <stdlib.h>
#include <string.h>

static int kev_lexgen_control_parse(FILE* input, KevParserState* parser_state);

static void kev_lexgen_set_opt_from_env(KevOptions* options, KevStringMap* env_var);
static void kev_lexgen_set_env_var_after(KevOptions* options, KevStringMap* env_var);
static void kev_lexgen_set_env_var_before(KevOptions* options, KevStringMap* env_var);
static void kev_lexgen_set_env_var_for_output(KevPatternBinary* binary_info, KevOutputFunc* func_group, KevStringMap* env_var);
static void fatal_error(char* info, char* info2);

void kev_lexgen_control(KevOptions* options) {
  /* show help */
  if (options->opts[KEV_LEXGEN_OPT_HELP]) {
    kev_lexgen_output_help();
    return;
  }

  KevParserState parser_state;
  if (!kev_lexgenparser_init(&parser_state)) {
    fatal_error("failed to initialize parser", NULL);
  }
  kev_lexgen_set_env_var_before(options, &parser_state.env_var);

  /* read lexical description file */
  FILE* input = fopen(options->strs[KEV_LEXGEN_INPUT_PATH], "r");
  if (!input) {
    kev_lexgenparser_destroy(&parser_state);
    fatal_error("can not open file: ", options->strs[KEV_LEXGEN_INPUT_PATH]);
  }
  int error_number = kev_lexgen_control_parse(input, &parser_state);
  fclose(input);
  if (error_number != 0) {
    fprintf(stderr, "%d error(s) detected.\n", error_number);
    exit(EXIT_FAILURE);
  }
  kev_lexgen_set_opt_from_env(options, &parser_state.env_var);
  kev_lexgen_set_env_var_after(options, &parser_state.env_var);

  /* convert */
  KevPatternBinary binary_info;
  kev_lexgen_convert(&binary_info, &parser_state);

  /* output transition table */
  FILE* output_src_and_tab = fopen(options->strs[KEV_LEXGEN_OUT_SRC_TAB_PATH], "w");
  if (!output_src_and_tab) {
    fatal_error("can not open file: ", options->strs[KEV_LEXGEN_OUT_SRC_TAB_PATH]);
  }
  KevOutputFunc func_group;
  kev_lexgen_output_set_func(&func_group, options->strs[KEV_LEXGEN_LANG_NAME]);
  kev_lexgen_set_env_var_for_output(&binary_info, &func_group, &parser_state.env_var);
  func_group.output_table(output_src_and_tab, &binary_info);
  func_group.output_pattern_mapping(output_src_and_tab, &binary_info);
  func_group.output_start(output_src_and_tab, &binary_info);
  kev_lexgen_output_src(output_src_and_tab, options, &parser_state.env_var);
  fclose(output_src_and_tab);

  /* compile source to archive file or shared object file if specified */
  if (options->opts[KEV_LEXGEN_OPT_STAGE] == KEV_LEXGEN_OPT_STA_ARC) {
    kev_lexgen_output_arc(options);
  } else if (options->opts[KEV_LEXGEN_OPT_STAGE] == KEV_LEXGEN_OPT_STA_SHA) {
    kev_lexgen_output_sha(options);
  }
  /* free resources */
  kev_lexgenparser_destroy(&parser_state);
  kev_lexgen_convert_destroy(&binary_info);
}

static void fatal_error(char* info, char* info2) {
  fputs("fatal: ", stderr);
  if (info)
    fputs(info, stderr);
  if (info2)
    fputs(info2, stderr);
  fputs("\nterminated\n", stderr);
  exit(EXIT_FAILURE);
}

static void kev_lexgen_set_env_var_for_output(KevPatternBinary* binary_info,
                                              KevOutputFunc* func_group, KevStringMap* env_var) {
  if (!kev_strmap_update_move(env_var, "callback-array",
      func_group->output_callback(binary_info))) {
    fatal_error("out of memory", NULL);
  }
  if (!kev_strmap_update_move(env_var, "info-array",
      func_group->output_info(binary_info))) {
    fatal_error("out of memory", NULL);
  }
  if (!kev_strmap_update_move(env_var, "macro-definition",
      func_group->output_macro(binary_info))) {
    fatal_error("out of memory", NULL);
  }
}

static int kev_lexgen_control_parse(FILE* input, KevParserState* parser_state) {
  KevLexGenLexer lex;
  KevLexGenToken token;
  if (!kev_lexgenlexer_init(&lex, input))
    fatal_error("kev_lexgenlexer_init() failed", NULL);
  while (!kev_lexgenlexer_next(&lex, &token))
    continue;
  int error_number = kev_lexgenparser_lex_src(&lex, &token, parser_state);
  kev_lexgenlexer_destroy(&lex);
  return error_number;
}


static void kev_lexgen_set_opt_from_env(KevOptions* options, KevStringMap* env_var) {
  KevStringMapNode* node = kev_strmap_search(env_var, "state-length");
  if (node) {
    int len = atoi(node->value);
    if (len != 16 && len != 8)
      fatal_error("state-length can not be: ", node->value);
    options->env_opts[KEV_LEXGEN_OPT_WIDTH] = len;
  } else {
    if (!kev_strmap_update(env_var, "state-length", "8")) {
      fatal_error("out of memory", NULL);
    }
    options->env_opts[KEV_LEXGEN_OPT_WIDTH] = 8;
  }

  node = kev_strmap_search(env_var, "charset");
  if (node) {
    if (strcmp("ascii", node->value) == 0) {
      options->env_opts[KEV_LEXGEN_OPT_CHARSET] = KEV_LEXGEN_OPT_CHARSET_ASCII;
      if (!kev_strmap_update(env_var, "charset-size", "128")) {
        fatal_error("out of memory", NULL);
      }
    } else if (strcmp("utf-8", node->value) == 0 || strcmp("UTF-8", node->value) == 0) {
      options->env_opts[KEV_LEXGEN_OPT_CHARSET] = KEV_LEXGEN_OPT_CHARSET_UTF8;
      if (!kev_strmap_update(env_var, "charset-size", "256")) {
        fatal_error("out of memory", NULL);
      }
    }
  } else {
    options->env_opts[KEV_LEXGEN_OPT_CHARSET] = KEV_LEXGEN_OPT_CHARSET_UTF8;
    if (!kev_strmap_update(env_var, "charset", "utf-8") ||
        !kev_strmap_update(env_var, "charset-size", "256")) {
      fatal_error("out of memory", NULL);
    }
  }
}

static void kev_lexgen_set_env_var_after(KevOptions* options, KevStringMap* env_var) {
  KevStringMapNode* node = kev_strmap_search(env_var, "state-length");
  node = kev_strmap_search(env_var, "charset");
  /* set charset-size */
  if (node) {
    if (strcmp("ascii", node->value) == 0) {
      if (!kev_strmap_update(env_var, "charset-size", "128")) {
        fatal_error("out of memory", NULL);
      }
    } else if (strcmp("utf-8", node->value) == 0 || strcmp("UTF-8", node->value) == 0) {
      if (!kev_strmap_update(env_var, "charset-size", "256")) {
        fatal_error("out of memory", NULL);
      }
    }
  } else {
    if (!kev_strmap_update(env_var, "charset-size", "256")) {
      fatal_error("out of memory", NULL);
    }
  }
}

static void kev_lexgen_set_env_var_before(KevOptions* options, KevStringMap* env_var) {
  if (options->strs[KEV_LEXGEN_OUT_INC_PATH]) {
    char* relpath = kev_get_relpath(options->strs[KEV_LEXGEN_OUT_SRC_TAB_PATH],
                                    options->strs[KEV_LEXGEN_OUT_INC_PATH]);
    if (!relpath)
      fatal_error("out of memory", NULL);
    if (!kev_strmap_update(env_var , "include-path", relpath))
      fatal_error("out of memory", NULL);
    free(relpath);
  }
  char* resources_dir = kev_get_lexgen_resources_dir();
  if (!resources_dir)
    fatal_error("out of memory", NULL);
  if (!kev_strmap_update(env_var, "import-path", resources_dir)) {
    fatal_error("failed to initialize environment variable \"import-path\"", NULL);
  }
  free(resources_dir);
  if (options->opts[KEV_LEXGEN_OPT_STAGE] == KEV_LEXGEN_OPT_STA_TAB) {
    if (!kev_strmap_update(env_var , "no-source", "enable"))
      fatal_error("out of memory", NULL);
  }
}
