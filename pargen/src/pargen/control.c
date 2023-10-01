#include "pargen/include/pargen/control.h"
#include "lexgen/include/lexgen/error.h"
#include "pargen/include/pargen/confhandle.h"
#include "pargen/include/pargen/convert.h"
#include "pargen/include/pargen/error.h"
#include "pargen/include/pargen/output.h"

#include <string.h>

static KevLRCollection* kev_pargen_control_generate_collec(KevPParserState* parser_state);
static KevLRTable* kev_pargen_control_generate_table(KevPParserState* parser_state, KevLRCollection* collec);
static KevFuncMap* kev_pargen_control_get_funcmap(KevPTableInfo* table_info, KevPOutputFuncGroup* func_group);


void kev_pargen_control(KevPOptions* options) {
  /* show help if specified */
  if (options->opts[KEV_PARGEN_OPT_HELP]) {
    kev_pargen_output_help();
    return;
  }

  KevPParserState parser_state;
  if (!kev_pargenparser_init(&parser_state)) {
    kev_throw_error("control:", "failed to initialise parser", NULL);
  }
  if (!kev_pargenparser_parse_file(&parser_state, options->strs[KEV_PARGEN_INPUT_PATH])) {
    kev_throw_error("control:", "can not open file: ", options->strs[KEV_PARGEN_INPUT_PATH]);
  }
  
  if (parser_state.err_count != 0) {
    char num_buf[256];
    sprintf(num_buf, "%d", (int)parser_state.err_count);
    kev_throw_error("control:", num_buf, " error(s) detected.");
  }

  KevLRCollection* collec = kev_pargen_control_generate_collec(&parser_state);
  KevLRTable* table = kev_pargen_control_generate_table(&parser_state, collec);
  KevPTableInfo table_info;
  kev_pargen_convert(&table_info, table, &parser_state);
  kev_lr_table_delete(table);
  kev_lr_collection_delete(collec);

  KevPOutputFuncGroup func_group;
  kev_pargen_output_set_func(&func_group, options->strs[KEV_PARGEN_LANG_NAME]);
  KevFuncMap* funcs = kev_pargen_control_get_funcmap(&table_info, &func_group);
  if (options->strs[KEV_PARGEN_OUT_SRC_PATH])
    kev_pargen_output(options->strs[KEV_PARGEN_OUT_SRC_PATH], options->strs[KEV_PARGEN_SRC_TMPL_PATH], parser_state.env_var, funcs);
  if (options->strs[KEV_PARGEN_OUT_INC_PATH])
    kev_pargen_output(options->strs[KEV_PARGEN_OUT_INC_PATH], options->strs[KEV_PARGEN_INC_TMPL_PATH], parser_state.env_var, funcs);
  kev_funcmap_delete(funcs);

  kev_pargenparser_destroy(&parser_state);
}

static KevFuncMap* kev_pargen_control_get_funcmap(KevPTableInfo* table_info, KevPOutputFuncGroup* func_group) {
  KevFuncMap* funcs = kev_funcmap_create();
  if (!funcs) {
    kev_throw_error("control:", "out of memory", NULL);
  }
  if (!kev_funcmap_insert(funcs, "callback-array",
      (void*)func_group->output_callback_array, table_info)) {
    kev_throw_error("control:", "out of memory", NULL);
  }
  if (!kev_funcmap_insert(funcs, "symbol-array",
      (void*)func_group->output_symbol_array, table_info)) {
    kev_throw_error("control:", "out of memory", NULL);
  }
  if (!kev_funcmap_insert(funcs, "start-state",
      (void*)func_group->output_start_state, table_info)) {
    kev_throw_error("control:", "out of memory", NULL);
  }
  if (!kev_funcmap_insert(funcs, "goto-table",
      (void*)func_group->output_goto_table, table_info)) {
    kev_throw_error("control:", "out of memory", NULL);
  }
  if (!kev_funcmap_insert(funcs, "action-table",
      (void*)func_group->output_action_table, table_info)) {
    kev_throw_error("control:", "out of memory", NULL);
  }
  if (!kev_funcmap_insert(funcs, "rule-info-array",
      (void*)func_group->output_rule_info_array, table_info)) {
    kev_throw_error("control:", "out of memory", NULL);
  }
  return funcs;
}

static KevLRCollection* kev_pargen_control_generate_collec(KevPParserState* parser_state) {
  KevLRCollection* collec = NULL;
  if (strcmp(parser_state->algorithm, "lalr") == 0) {
    collec = kev_lr_collection_create_lalr(parser_state->start,
                                           (KevSymbol**)kev_addrarray_raw_array(parser_state->end_symbols),
                                           kev_addrarray_size(parser_state->end_symbols));
  } else if (strcmp(parser_state->algorithm, "lr1") == 0) {
    collec = kev_lr_collection_create_lr1(parser_state->start,
                                           (KevSymbol**)kev_addrarray_raw_array(parser_state->end_symbols),
                                           kev_addrarray_size(parser_state->end_symbols));
  } else if (strcmp(parser_state->algorithm, "slr") == 0) {
    collec = kev_lr_collection_create_slr(parser_state->start,
                                           (KevSymbol**)kev_addrarray_raw_array(parser_state->end_symbols),
                                           kev_addrarray_size(parser_state->end_symbols));
  } else {
    kev_throw_error("control:", "unsupported algorithm: ", parser_state->algorithm);
  }
  if (!collec) {
    kev_throw_error("control:", "error occurred when generating lr collection", NULL);
  }
  return collec;
}

static KevLRTable* kev_pargen_control_generate_table(KevPParserState* parser_state, KevLRCollection* collec) {
  KevLRConflictHandler* handler = kev_pargen_confhandle_get_handler(parser_state);
  KevLRTable* table = kev_lr_table_create(collec, handler);
  if (!table) {
    kev_throw_error("control:", "failed to generate table", NULL);
  }
  if (kev_lr_table_get_conflict(table)) {
    kev_throw_error("control:", "there are unresolved conflicts in the table, ", "failed to generate table");
  }
  kev_pargen_confhandle_delete(handler);
  return table;
}
