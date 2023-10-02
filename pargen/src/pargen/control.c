#include "pargen/include/pargen/control.h"
#include "pargen/include/pargen/confhandle.h"
#include "pargen/include/pargen/convert.h"
#include "pargen/include/pargen/error.h"
#include "pargen/include/pargen/output.h"

#include <string.h>

static KevLRCollection* kev_pargen_control_generate_collec(KevPParserState* parser_state);
static KevLRTable* kev_pargen_control_generate_table(KevPParserState* parser_state, KevLRCollection* collec);
static KevFuncMap* kev_pargen_control_get_funcmap(KevPTableInfo* table_info, KevPOutputFuncGroup* func_group);

static void kev_pargen_control_set_env_var_pre(KevStringMap* env_var);
static void kev_pargen_control_set_env_var_post(KevStringMap* env_var, KevPTableInfo* table_info);


void kev_pargen_control(KevPOptions* options) {
  /* show help if specified */
  if (options->opts[KEV_PARGEN_OPT_HELP]) {
    kev_pargen_output_help();
    return;
  }

  /* prepare to parse input file */
  KevPParserState parser_state;
  if (!kev_pargenparser_init(&parser_state)) {
    kev_throw_error("control:", "failed to initialise parser", NULL);
  }

  /* predefine variables */
  kev_pargen_control_set_env_var_pre(parser_state.env_var);

  /* parse input file */
  if (!kev_pargenparser_parse_file(&parser_state, options->strs[KEV_PARGEN_INPUT_PATH])) {
    kev_throw_error("control:", "can not open file: ", options->strs[KEV_PARGEN_INPUT_PATH]);
  }
  
  /* if there is any error, report and abort */
  if (parser_state.err_count != 0) {
    char num_buf[256];
    sprintf(num_buf, "%d", (int)parser_state.err_count);
    kev_pargenparser_destroy(&parser_state);
    kev_throw_error("control:", num_buf, " error(s) detected.");
  }

  /* construct lr collection */
  KevLRCollection* collec = kev_pargen_control_generate_collec(&parser_state);
  /* generate lr table */
  KevLRTable* table = kev_pargen_control_generate_table(&parser_state, collec);
  /* print lr information if specified */
  kev_pargen_output_lrinfo(options->strs[KEV_PARGEN_LRINFO_COLLEC_PATH],
                           options->strs[KEV_PARGEN_LRINFO_ACTION_PATH],
                           options->strs[KEV_PARGEN_LRINFO_GOTO_PATH],
                           options->strs[KEV_PARGEN_LRINFO_SYMBOL_PATH],
                           collec, table);
  /* abort if exist conflict */
  if (kev_lr_table_get_conflict(table))
    kev_throw_error("control:", "there are unresolved conflicts, ", "failed to generate table");
  /* convert lr information to a simpler representation */
  KevPTableInfo table_info;
  kev_pargen_convert(&table_info, table, &parser_state);
  /* release resources that are no longer needed */
  kev_lr_table_delete(table);
  kev_lr_collection_delete(collec);
  /* set variables corresponding to fields of 'table_info' */
  kev_pargen_control_set_env_var_post(parser_state.env_var, &table_info);

  /* set function group, which contains functions that output the lr table
   * to a source file. */
  KevPOutputFuncGroup func_group;
  kev_pargen_output_set_func(&func_group, options->strs[KEV_PARGEN_LANG_NAME]);
  KevFuncMap* funcs = kev_pargen_control_get_funcmap(&table_info, &func_group);
  /* generate source code from a template file if output path is specified */
  if (options->strs[KEV_PARGEN_OUT_SRC_PATH])
    kev_pargen_output(options->strs[KEV_PARGEN_OUT_SRC_PATH], options->strs[KEV_PARGEN_SRC_TMPL_PATH], parser_state.env_var, funcs);
  if (options->strs[KEV_PARGEN_OUT_INC_PATH])
    kev_pargen_output(options->strs[KEV_PARGEN_OUT_INC_PATH], options->strs[KEV_PARGEN_INC_TMPL_PATH], parser_state.env_var, funcs);
  /* release resources */
  kev_funcmap_delete(funcs);
  kev_pargen_convert_destroy(&table_info);
  /* release resources */
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
  if (!kev_funcmap_insert(funcs, "state-symbol-mapping",
      (void*)func_group->output_state_symbol_mapping, table_info)) {
    kev_throw_error("control:", "out of memory", NULL);
  }
  return funcs;
}

static KevLRCollection* kev_pargen_control_generate_collec(KevPParserState* parser_state) {
  KevLRCollection* collec = NULL;

  if (!parser_state->start) {
    kev_throw_error("control:", "start symbol should be specified", NULL);
  }

  if (kev_addrarray_size(parser_state->end_symbols) == 0) {
    kev_throw_error("control:", "end symbol should be specified", NULL);
  }

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
  kev_pargen_confhandle_delete(handler);
  if (!table) {
    kev_throw_error("control:", "failed to generate table", NULL);
  }
  return table;
}

static void kev_pargen_control_set_env_var_post(KevStringMap* env_var, KevPTableInfo* table_info) {
  char buf[100];
  sprintf(buf, "%d", (int)table_info->state_no);
  if (!kev_strmap_update(env_var, "state-number",buf))
    kev_throw_error("control:", "out of memory", ", falied to set variable 'state-number'");
  sprintf(buf, "%d", (int)table_info->action_col_no);
  if (!kev_strmap_update(env_var, "action-column",buf))
    kev_throw_error("control:", "out of memory", ", falied to set variable 'action-column'");
  sprintf(buf, "%d", (int)table_info->goto_col_no);
  if (!kev_strmap_update(env_var, "goto-column",buf))
    kev_throw_error("control:", "out of memory", ", falied to set variable 'goto-column'");
  /* goto-column is equal to symbol-number */
  if (!kev_strmap_update(env_var, "symbol-number",buf))
    kev_throw_error("control:", "out of memory", ", falied to set variable 'symbol-number'");
  sprintf(buf, "%d", (int)table_info->rule_no);
  if (!kev_strmap_update(env_var, "rule-number",buf))
    kev_throw_error("control:", "out of memory", ", falied to set variable 'rule-number'");
}

static void kev_pargen_control_set_env_var_pre(KevStringMap* env_var) {
  char buf[100];
  sprintf(buf, "%d", KEV_LR_ACTION_SHI);
  if (!kev_strmap_update(env_var, "lr-action-shift",buf))
    kev_throw_error("control:", "out of memory", ", falied to set variable 'lr-action-shift'");
  sprintf(buf, "%d", KEV_LR_ACTION_RED);
  if (!kev_strmap_update(env_var, "lr-action-reduce",buf))
    kev_throw_error("control:", "out of memory", ", falied to set variable 'lr-action-reduce'");
  sprintf(buf, "%d", KEV_LR_ACTION_ACC);
  if (!kev_strmap_update(env_var, "lr-action-accept",buf))
    kev_throw_error("control:", "out of memory", ", falied to set variable 'lr-action-accept'");
  sprintf(buf, "%d", KEV_LR_ACTION_CON);
  if (!kev_strmap_update(env_var, "lr-action-conflict",buf))
    kev_throw_error("control:", "out of memory", ", falied to set variable 'lr-action-conflict'");
  sprintf(buf, "%d", KEV_LR_ACTION_ERR);
  if (!kev_strmap_update(env_var, "lr-action-error",buf))
    kev_throw_error("control:", "out of memory", ", falied to set variable 'lr-action-error'");
}
