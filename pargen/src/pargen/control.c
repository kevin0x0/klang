#include "pargen/include/pargen/control.h"
#include "pargen/include/pargen/confhandle.h"
#include "pargen/include/pargen/convert.h"
#include "pargen/include/pargen/dir.h"
#include "pargen/include/pargen/error.h"
#include "pargen/include/pargen/output.h"
#include "pargen/include/parser/symtable.h"
#include "utils/include/string/kev_string.h"

#include <string.h>

static KlrCollection* kev_pargen_control_generate_collec(KevPParserState* parser_state);
static KlrTable* kev_pargen_control_generate_table(KevPParserState* parser_state, KlrCollection* collec);
static KevFuncMap* kev_pargen_control_get_funcmap(KevPTableInfo* table_info, KevPOutputFuncGroup* func_group);
static void kev_pargen_control_parse(KevPParserState* parser_state, const char* filepath);

static size_t kev_pargen_control_set_vars_pre(KevPParserState* parser_state, KevPOptions* options);
static void kev_pargen_control_set_vars_post(KevPParserState* parser_state, KevPTableInfo* table_info, size_t env_id);


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
  size_t env_id = kev_pargen_control_set_vars_pre(&parser_state, options);

  /* parse input file */
  kev_pargen_control_parse(&parser_state, options->strs[KEV_PARGEN_INPUT_PATH]);
  
  /* if there is any error, report and abort */
  if (parser_state.err_count != 0) {
    char num_buf[256];
    sprintf(num_buf, "%d", (int)parser_state.err_count);
    kev_pargenparser_destroy(&parser_state);
    kev_throw_error("control:", num_buf, " error(s) detected.");
  }

  /* construct lr collection */
  KlrCollection* collec = kev_pargen_control_generate_collec(&parser_state);
  /* generate lr table */
  KlrTable* table = kev_pargen_control_generate_table(&parser_state, collec);
  /* print lr information if specified */
  kev_pargen_output_lrinfo(options->strs[KEV_PARGEN_LRINFO_COLLEC_PATH],
                           options->strs[KEV_PARGEN_LRINFO_ACTION_PATH],
                           options->strs[KEV_PARGEN_LRINFO_GOTO_PATH],
                           options->strs[KEV_PARGEN_LRINFO_SYMBOL_PATH],
                           collec, table);
  /* abort if exist conflict */
  if (klr_table_get_conflict(table))
    kev_throw_error("control:", "there are unresolved conflicts, ", "failed to generate table");
  /* convert lr information to a simpler representation */
  KevPTableInfo table_info;
  kev_pargen_convert(&table_info, table, &parser_state);
  /* release resources that are no longer needed */
  klr_table_delete(table);
  klr_collection_delete(collec);
  /* set variables corresponding to fields of 'table_info' */
  kev_pargen_control_set_vars_post(&parser_state, &table_info, env_id);

  /* set function group, which contains functions that output the lr table
   * to a source file. */
  KevPOutputFuncGroup func_group;
  kev_pargen_output_set_func(&func_group, options->strs[KEV_PARGEN_LANG_NAME]);
  KevFuncMap* funcs = kev_pargen_control_get_funcmap(&table_info, &func_group);
  /* generate source code from a template file if output path is specified */
//  if (options->strs[KEV_PARGEN_OUT_SRC_PATH])
 //   kev_pargen_output(options->strs[KEV_PARGEN_OUT_SRC_PATH], options->strs[KEV_PARGEN_SRC_TMPL_PATH], parser_state.vars, funcs);
  //if (options->strs[KEV_PARGEN_OUT_INC_PATH])
   // kev_pargen_output(options->strs[KEV_PARGEN_OUT_INC_PATH], options->strs[KEV_PARGEN_INC_TMPL_PATH], parser_state.vars, funcs);
  /* release resources */
  kev_funcmap_delete(funcs);
  kev_pargen_convert_destroy(&table_info);
  /* release resources */
  kev_pargenparser_destroy(&parser_state);
}

static void kev_pargen_control_parse(KevPParserState* parser_state, const char* filepath) {
  if (!kev_pargenparser_parse_file(parser_state, filepath)) {
    kev_throw_error("control:", "can not open file: ", filepath);
  }
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

static KlrCollection* kev_pargen_control_generate_collec(KevPParserState* parser_state) {
  KlrCollection* collec = NULL;

  if (!parser_state->start) {
    kev_throw_error("control:", "start symbol should be specified", NULL);
  }

  if (karray_size(parser_state->end_symbols) == 0) {
    kev_throw_error("control:", "end symbol should be specified", NULL);
  }

  if (strcmp(parser_state->algorithm, "lalr") == 0) {
    collec = klr_collection_create_lalr(parser_state->start,
                                           (KlrSymbol**)karray_raw(parser_state->end_symbols),
                                           karray_size(parser_state->end_symbols));
  } else if (strcmp(parser_state->algorithm, "lr1") == 0) {
    collec = klr_collection_create_lr1(parser_state->start,
                                           (KlrSymbol**)karray_raw(parser_state->end_symbols),
                                           karray_size(parser_state->end_symbols));
  } else if (strcmp(parser_state->algorithm, "slr") == 0) {
    collec = klr_collection_create_slr(parser_state->start,
                                           (KlrSymbol**)karray_raw(parser_state->end_symbols),
                                           karray_size(parser_state->end_symbols));
  } else {
    kev_throw_error("control:", "unsupported algorithm: ", parser_state->algorithm);
  }
  if (!collec) {
    kev_throw_error("control:", "error occurred when generating lr collection", NULL);
  }
  return collec;
}

static KlrTable* kev_pargen_control_generate_table(KevPParserState* parser_state, KlrCollection* collec) {
  KlrConflictHandler* handler = kev_pargen_confhandle_get_handler(parser_state);
  KlrTable* table = klr_table_create(collec, handler);
  kev_pargen_confhandle_delete(handler);
  if (!table) {
    kev_throw_error("control:", "failed to generate table", NULL);
  }
  return table;
}

static void kev_pargen_control_set_vars_post(KevPParserState* parser_state, KevPTableInfo* table_info, size_t env_id) {
  KevSymTable* env = karray_access(parser_state->symtables, env_id);
  char* endptr = NULL;
  KevSymTableNode* node = kev_symtable_search(env, "lr");
  if (!node) {
    kev_throw_error("control:", "can not find variable ", "'lr'");
  }
  size_t lr_id = (size_t)strtoull(node->value, &endptr, 10);
  if (*endptr != '\0' || lr_id >= karray_size(parser_state->symtables)) {
    kev_throw_error("control:", "variable 'lr' was changed", NULL);
  }
  KevSymTable* lr = karray_access(parser_state->symtables, lr_id);
  char buf[100];
  sprintf(buf, "%d", (int)table_info->state_no);
  if (!kev_symtable_update(lr, "state-number", buf))
    kev_throw_error("control:", "out of memory", ", falied to set variable 'state-number'");
  sprintf(buf, "%d", (int)table_info->action_col_no);
  if (!kev_symtable_update(lr, "action-column", buf))
    kev_throw_error("control:", "out of memory", ", falied to set variable 'action-column'");
  sprintf(buf, "%d", (int)table_info->goto_col_no);
  if (!kev_symtable_update(lr, "goto-column", buf))
    kev_throw_error("control:", "out of memory", ", falied to set variable 'goto-column'");
  /* goto-column is equal to symbol-number */
  if (!kev_symtable_update(lr, "symbol-number", buf))
    kev_throw_error("control:", "out of memory", ", falied to set variable 'symbol-number'");
  sprintf(buf, "%d", (int)table_info->rule_no);
  if (!kev_symtable_update(lr, "rule-number", buf))
    kev_throw_error("control:", "out of memory", ", falied to set variable 'rule-number'");
}

static size_t kev_pargen_control_set_vars_pre(KevPParserState* parser_state, KevPOptions* options) {
  char buf[100];
  /* create symbol table env, opt and lr */
  size_t env_id = karray_size(parser_state->symtables);
  KevSymTable* env = kev_symtable_create(env_id, env_id);
  if(!env || !karray_push_back(parser_state->symtables, env))
    kev_throw_error("control:", "out of memory", NULL);
  KevSymTable* opt = kev_symtable_create(karray_size(parser_state->symtables), env_id);
  size_t opt_id = kev_symtable_get_self_id(opt);
  if(!opt || !karray_push_back(parser_state->symtables, opt))
    kev_throw_error("control:", "out of memory", NULL);
  KevSymTable* lr = kev_symtable_create(karray_size(parser_state->symtables), env_id);
  size_t lr_id = kev_symtable_get_self_id(lr);
  if(!lr || !karray_push_back(parser_state->symtables, lr))
    kev_throw_error("control:", "out of memory", NULL);

  parser_state->curr_symtbl = env_id;

  /* set variables in 'env' */
  sprintf(buf, "%d", (int)env_id);
  if (!kev_symtable_insert(env, "env", buf))
    kev_throw_error("control:", "out of memory", ", falied to set variable 'env'");
  sprintf(buf, "%d", (int)opt_id);
  if (!kev_symtable_insert(env, "opt", buf))
    kev_throw_error("control:", "out of memory", ", falied to set variable 'opt'");
  sprintf(buf, "%d", (int)lr_id);
  if (!kev_symtable_insert(env, "lr", buf))
    kev_throw_error("control:", "out of memory", ", falied to set variable 'lr'");
  char* resources = kev_get_pargen_resources_dir();
  char* klr_dir = kev_str_concat(resources, "language/");
  free(resources);
  if (!klr_dir || !kev_symtable_insert_move(env, "res", klr_dir))
    kev_throw_error("control:", "out of memory", ", falied to set variable 'res'");


  /* set variables in 'lr' */
  sprintf(buf, "%d", KLR_ACTION_SHI);
  if (!kev_symtable_insert(lr, "action-shift", buf))
    kev_throw_error("control:", "out of memory", ", falied to set variable 'action-shift'");
  sprintf(buf, "%d", KLR_ACTION_RED);
  if (!kev_symtable_insert(lr, "action-reduce", buf))
    kev_throw_error("control:", "out of memory", ", falied to set variable 'action-reduce'");
  sprintf(buf, "%d", KLR_ACTION_ACC);
  if (!kev_symtable_insert(lr, "action-accept", buf))
    kev_throw_error("control:", "out of memory", ", falied to set variable 'action-accept'");
  sprintf(buf, "%d", KLR_ACTION_CON);
  if (!kev_symtable_insert(lr, "action-conflict", buf))
    kev_throw_error("control:", "out of memory", ", falied to set variable 'action-conflict'");
  sprintf(buf, "%d", KLR_ACTION_ERR);
  if (!kev_symtable_insert(lr, "action-error", buf))
    kev_throw_error("control:", "out of memory", ", falied to set variable 'action-error'");

  /* set variables in 'opt' */
  if (!kev_symtable_insert(opt, "language", options->strs[KEV_PARGEN_LANG_NAME]))
    kev_throw_error("control:", "out of memory", ", falied to set variable 'language'");
}
