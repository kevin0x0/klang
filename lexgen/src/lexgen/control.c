#include "lexgen/include/lexgen/control.h"
#include "lexgen/include/lexgen/output.h"
#include "lexgen/include/lexgen/error.h"
#include "lexgen/include/lexgen/dir.h"
#include "utils/include/hashmap/func_map.h"

#include <stdlib.h>
#include <string.h>

static int kev_lexgen_control_parse(FILE* input, KevLParserState* parser_state);
/* Set environment variable after parsing the lexical description file.
 * It set some variables according to some other variables set in the file. */
static void kev_lexgen_control_set_env_var_post(KevStringMap* env_var);
/* Set environment variable before parsing the lexical description file. */
static void kev_lexgen_control_set_env_var_pre(KevLOptions* options, KevStringMap* env_var);
/* Set variables whose value is the transition table or token infos after
 * generating DFA and converting it to table. These variables used in the
 * template file. */
static KevFuncMap* kev_lexgen_control_get_funcmap(KevLTableInfos* table_info, KevLOutputFuncGroup* func_group);

void kev_lexgen_control(KevLOptions* options) {
  /* show help if specified */
  if (options->opts[KEV_LEXGEN_OPT_HELP]) {
    kev_lexgen_output_help();
    return;
  }

  KevLParserState parser_state;
  if (!kev_lexgenparser_init(&parser_state)) {
    kev_throw_error("control:", "failed to initialize parser", NULL);
  }
  kev_lexgen_control_set_env_var_pre(options, &parser_state.env_var);

  /* read lexical description file */
  FILE* input = fopen(options->strs[KEV_LEXGEN_INPUT_PATH], "r");
  if (!input) {
    kev_lexgenparser_destroy(&parser_state);
    kev_throw_error("control:", "can not open file: ", options->strs[KEV_LEXGEN_INPUT_PATH]);
  }
  int error_number = kev_lexgen_control_parse(input, &parser_state);
  fclose(input);
  if (error_number != 0) {
    char num_buf[256];  /* size 256 should be enough for integer */
    sprintf(num_buf, "%d", error_number);
    kev_throw_error("control: ", num_buf, " error(s) detected.");
  }
  kev_lexgen_control_set_env_var_post(&parser_state.env_var);

  /* convert */
  KevLTableInfos table_info;
  kev_lexgen_convert(&table_info, &parser_state);

  /* output transition table */
  KevLOutputFuncGroup func_group;
  kev_lexgen_output_set_func(&func_group, options->strs[KEV_LEXGEN_LANG_NAME]);
  KevFuncMap* funcs = kev_lexgen_control_get_funcmap(&table_info, &func_group);
  if (options->strs[KEV_LEXGEN_OUT_SRC_PATH])
    kev_lexgen_output(options->strs[KEV_LEXGEN_OUT_SRC_PATH], options->strs[KEV_LEXGEN_SRC_TMPL_PATH], &parser_state.env_var, funcs);
  if (options->strs[KEV_LEXGEN_OUT_INC_PATH])
    kev_lexgen_output(options->strs[KEV_LEXGEN_OUT_INC_PATH], options->strs[KEV_LEXGEN_INC_TMPL_PATH], &parser_state.env_var, funcs);
  kev_funcmap_delete(funcs);

  /* free resources */
  kev_lexgenparser_destroy(&parser_state);
  kev_lexgen_convert_destroy(&table_info);
}

static KevFuncMap* kev_lexgen_control_get_funcmap(KevLTableInfos* table_info, KevLOutputFuncGroup* func_group) {
  KevFuncMap* funcs = kev_funcmap_create();
  if (!funcs) {
    kev_throw_error("control:", "out of memory", NULL);
  }
  if (!kev_funcmap_insert(funcs, "callback-array",
      (void*)func_group->output_callback, table_info)) {
    kev_throw_error("control:", "out of memory", NULL);
  }
  if (!kev_funcmap_insert(funcs, "info-array",
      (void*)func_group->output_info, table_info)) {
    kev_throw_error("control:", "out of memory", NULL);
  }
  if (!kev_funcmap_insert(funcs, "macro-definition",
      (void*)func_group->output_macro, table_info)) {
    kev_throw_error("control:", "out of memory", NULL);
  }
  if (!kev_funcmap_insert(funcs, "transition-table",
      (void*)func_group->output_table, table_info)) {
    kev_throw_error("control:", "out of memory", NULL);
  }
  if (!kev_funcmap_insert(funcs, "pattern-mapping",
      (void*)func_group->output_pattern_mapping, table_info)) {
    kev_throw_error("control:", "out of memory", NULL);
  }
  if (!kev_funcmap_insert(funcs, "start-state",
      (void*)func_group->output_start, table_info)) {
    kev_throw_error("control:", "out of memory", NULL);
  }
  return funcs;
}

static int kev_lexgen_control_parse(FILE* input, KevLParserState* parser_state) {
  KevLLexer lex;
  KevLToken token;
  if (!kev_lexgenlexer_init(&lex, input))
    kev_throw_error("control:", "kev_lexgenlexer_init() failed", NULL);
  while (!kev_lexgenlexer_next(&lex, &token))
    continue;
  int error_number = kev_lexgenparser_lex_src(&lex, &token, parser_state);
  kev_lexgenlexer_destroy(&lex);
  return error_number;
}

static void kev_lexgen_control_set_env_var_post(KevStringMap* env_var) {
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

static void kev_lexgen_control_set_env_var_pre(KevLOptions* options, KevStringMap* env_var) {
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

  /* preset charset */
  if (!kev_strmap_update(env_var, "encoding", "utf-8")) {
    kev_throw_error("control:", "out of memory", NULL);
  }

  /* preset state-length */
  if (!kev_strmap_update(env_var, "state-length", "8")) {
    kev_throw_error("control:", "out of memory", NULL);
  }
}
