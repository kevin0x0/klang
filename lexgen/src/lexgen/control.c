#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "lexgen/include/lexgen/control.h"
#include "lexgen/include/lexgen/convert.h"
#include "lexgen/include/lexgen/output.h"
#include "lexgen/include/lexgen/error.h"
#include "utils/include/os_spec/dir.h"

#include <stdlib.h>
#include <string.h>

static int kev_lexgen_control_parse(FILE* input, KevParserState* parser_state);
/* Set environment variable after parsing the lexical description file.
 * It set some variables according to some other variables set in the file. */
static void kev_lexgen_control_set_env_var_after(KevOptions* options, KevStringMap* env_var);
/* Set environment variable before parsing the lexical description file. */
static void kev_lexgen_control_set_env_var_before(KevOptions* options, KevStringMap* env_var);
/* Set variables whose value is the transition table or token infos after
 * generating DFA and converting it to table. These variables used in the
 * template file. */
static void kev_lexgen_control_set_env_var_for_output(KevPatternBinary* binary_info, KevOutputFunc* func_group, KevStringMap* env_var);

void kev_lexgen_control(KevOptions* options) {
  /* show help if specified */
  if (options->opts[KEV_LEXGEN_OPT_HELP]) {
    kev_lexgen_output_help();
    return;
  }

  KevParserState parser_state;
  if (!kev_lexgenparser_init(&parser_state)) {
    kev_throw_error("control:", "failed to initialize parser", NULL);
  }
  kev_lexgen_control_set_env_var_before(options, &parser_state.env_var);

  /* read lexical description file */
  FILE* input = fopen(options->strs[KEV_LEXGEN_INPUT_PATH], "r");
  if (!input) {
    kev_lexgenparser_destroy(&parser_state);
    kev_throw_error("control:", "can not open file: ", options->strs[KEV_LEXGEN_INPUT_PATH]);
  }
  int error_number = kev_lexgen_control_parse(input, &parser_state);
  fclose(input);
  if (error_number != 0) {
    fprintf(stderr, "%d error(s) detected.\n", error_number);
    exit(EXIT_FAILURE);
  }
  kev_lexgen_control_set_env_var_after(options, &parser_state.env_var);

  /* convert */
  KevPatternBinary binary_info;
  kev_lexgen_convert(&binary_info, &parser_state);

  /* output transition table */
  FILE* output = fopen(options->strs[KEV_LEXGEN_OUT_SRC_PATH], "w");
  if (!output) {
    kev_throw_error("control:", "can not open file: ", options->strs[KEV_LEXGEN_OUT_SRC_PATH]);
  }
  KevOutputFunc func_group;
  kev_lexgen_output_set_func(&func_group, options->strs[KEV_LEXGEN_LANG_NAME]);
  kev_lexgen_control_set_env_var_for_output(&binary_info, &func_group, &parser_state.env_var);
  func_group.output_table(output, &binary_info);
  func_group.output_pattern_mapping(output, &binary_info);
  func_group.output_start(output, &binary_info);
  kev_lexgen_output_src(output, options, &parser_state.env_var);
  fclose(output);

  /* free resources */
  kev_lexgenparser_destroy(&parser_state);
  kev_lexgen_convert_destroy(&binary_info);
}

static void kev_lexgen_control_set_env_var_for_output(KevPatternBinary* binary_info,
                                              KevOutputFunc* func_group, KevStringMap* env_var) {
  if (!kev_strmap_update_move(env_var, "callback-array",
      func_group->output_callback(binary_info))) {
    kev_throw_error("control:", "out of memory", NULL);
  }
  if (!kev_strmap_update_move(env_var, "info-array",
      func_group->output_info(binary_info))) {
    kev_throw_error("control:", "out of memory", NULL);
  }
  if (!kev_strmap_update_move(env_var, "macro-definition",
      func_group->output_macro(binary_info))) {
    kev_throw_error("control:", "out of memory", NULL);
  }
}

static int kev_lexgen_control_parse(FILE* input, KevParserState* parser_state) {
  KevLexGenLexer lex;
  KevLexGenToken token;
  if (!kev_lexgenlexer_init(&lex, input))
    kev_throw_error("control:", "kev_lexgenlexer_init() failed", NULL);
  while (!kev_lexgenlexer_next(&lex, &token))
    continue;
  int error_number = kev_lexgenparser_lex_src(&lex, &token, parser_state);
  kev_lexgenlexer_destroy(&lex);
  return error_number;
}

static void kev_lexgen_control_set_env_var_after(KevOptions* options, KevStringMap* env_var) {
  KevStringMapNode* node = kev_strmap_search(env_var, "state-length");
  /* check state-length */
  if (node) {
    int len = atoi(node->value);
    if (len != 16 && len != 8)
      kev_throw_error("control:", "state-length can not be: ", node->value);
  } else {
    kev_throw_error("control:", "internal error: ", "can not find variable state-length");
  }
  /* set alphabet-size */
  node = kev_strmap_search(env_var, "encoding");
  if (node) {
    if (strcmp("ascii", node->value) == 0) {
      if (!kev_strmap_update(env_var, "alphabet-size", "128")) {
        kev_throw_error("control:", "out of memory", NULL);
      }
    } else if (strcmp("utf-8", node->value) == 0 || strcmp("UTF-8", node->value) == 0) {
      if (!kev_strmap_update(env_var, "alphabet-size", "256")) {
        kev_throw_error("control:", "out of memory", NULL);
      }
    }
  } else {
    kev_throw_error("control:", "internal error: ", "can not find variable state-length");
  }
}

static void kev_lexgen_control_set_env_var_before(KevOptions* options, KevStringMap* env_var) {
  /* set include path for C/C++ header */
  if (options->strs[KEV_LEXGEN_OUT_INC_PATH]) {
    char* relpath = kev_get_relpath(options->strs[KEV_LEXGEN_OUT_SRC_PATH],
                                    options->strs[KEV_LEXGEN_OUT_INC_PATH]);
    if (!relpath)
      kev_throw_error("control:", "out of memory", NULL);
    if (!kev_strmap_update(env_var , "include-path", relpath))
      kev_throw_error("control:", "out of memory", NULL);
    free(relpath);
  }

  /* set import path, this is the base directory used by
   * import statement in lexical description file. */
  char* resources_dir = kev_get_lexgen_resources_dir();
  if (!resources_dir)
    kev_throw_error("control:", "out of memory", NULL);
  if (!kev_strmap_update(env_var, "import-path", resources_dir)) {
    kev_throw_error("control:", "failed to initialize environment variable \"import-path\"", NULL);
  }
  free(resources_dir);

  /* these variables are used to control whether to generate source */
  if (options->opts[KEV_LEXGEN_OPT_TAB_ONLY] == KEV_LEXGEN_OPT_TRUE) {
    /* "The values of these two variables are not important,
     * it is only necessary to ensure that these two variables
     * have been defined. */
    if (!kev_strmap_update(env_var , "no-source", "enable") ||
        !kev_strmap_update(env_var , "no-header", "enable"))
      kev_throw_error("control:", "out of memory", NULL);
  }

  /* preset charset */
  if (!kev_strmap_update(env_var, "encoding", "utf-8")) {
    kev_throw_error("control:", "out of memory", NULL);
  }

  /* preset state-length */
  if (!kev_strmap_update(env_var, "state-length", "8")) {
    kev_throw_error("control:", "out of memory", NULL);
  }
}
