#include "pargen/include/pargen/convert.h"
#include "lexgen/include/lexgen/error.h"

#include <stdlib.h>
#include <string.h>


static void kev_pargen_convert_goto(KevPTableInfo* table_info, KevLRTable* table);
static void kev_pargen_convert_action(KevPTableInfo* table_info, KevLRTable* table);
static void kev_pargen_convert_rules_info(KevPTableInfo* table_info, KevPParserState* parser_state);
static void kev_pargen_convert_symbols(KevPTableInfo* table_info, KevPParserState* parser_state);


void kev_pargen_convert(KevPTableInfo* table_info, KevLRTable* table, KevPParserState* parser_state) {
  table_info->terminal_no = kev_lr_table_get_terminal_no(table);
  table_info->state_no = kev_lr_table_get_state_no(table);
  table_info->symbol_no = kev_lr_table_get_symbol_no(table);
  table_info->start_state = kev_lr_table_get_start_state(table);
  kev_pargen_convert_goto(table_info, table);
  kev_pargen_convert_action(table_info, table);
  kev_pargen_convert_rules_info(table_info, parser_state);
  kev_pargen_convert_symbols(table_info, parser_state);
}

static void kev_pargen_convert_goto(KevPTableInfo* table_info, KevLRTable* table) {
  table_info->goto_table = (int**)malloc(table_info->state_no * sizeof (int*));
  int* gotos = (int*)malloc(table_info->state_no * table_info->symbol_no * sizeof (int));
  if (!gotos || !table_info->goto_table) {
    kev_throw_error("convert:", "out of memory", NULL);
  }
  table_info->goto_table[0] = gotos;
  for (size_t i = 1; i < table_info->state_no; ++i)
    table_info->goto_table[i] = table_info->goto_table[i - 1] + table_info->symbol_no;

  for (size_t i = 0; i < table_info->state_no; ++i) {
    for (size_t j = 0; j < table_info->symbol_no; ++j)
      table_info->goto_table[i][j] = kev_lr_table_get_goto(table, i, j);
  }
}

static void kev_pargen_convert_action(KevPTableInfo* table_info, KevLRTable* table) {
  table_info->action_table = (KevPActionEntry**)malloc(table_info->state_no * sizeof (KevPActionEntry*));
  KevPActionEntry* actions = (KevPActionEntry*)malloc(table_info->state_no * table_info->symbol_no * sizeof (KevPActionEntry));
  if (!actions || !table_info->action_table) {
    kev_throw_error("convert:", "out of memory", NULL);
  }
  table_info->action_table[0] = actions;
  for (size_t i = 1; i < table_info->state_no; ++i) {
    table_info->action_table[i] = table_info->action_table[i - 1] + table_info->symbol_no;
  }

  for (size_t i = 0; i < table_info->state_no; ++i) {
    for (size_t j = 0; j < table_info->symbol_no; ++j) {
      table_info->action_table[i][j].action = kev_lr_table_get_action(table, i, j);
      switch (table_info->action_table[i][j].action) {
        case KEV_LR_ACTION_RED:
          table_info->action_table[i][j].info = kev_lr_rule_get_id(kev_lr_table_get_action_info(table, i, j)->rule);
          break;
        case KEV_LR_ACTION_SHI:
          table_info->action_table[i][j].info = kev_lr_table_get_action_info(table, i, j)->itemset_id;
          break;
        default:
          table_info->action_table[i][j].info = -1;
          break;
      }
    }
  }
}

static void kev_pargen_convert_rules_info(KevPTableInfo* table_info, KevPParserState* parser_state) {
  table_info->rule_no = kev_addrarray_size(parser_state->rules);
  table_info->rules_info = (KevPRuleInfo*)malloc(table_info->rule_no * sizeof(KevPRuleInfo));
  if (!table_info->rules_info) {
    kev_throw_error("convert:", "out of memory", NULL);
  }
  KevPRuleInfo* rules_info = table_info->rules_info;
  for (size_t i = 0; i < table_info->rule_no; ++i) {
    KevRule* rule = (KevRule*)kev_addrarray_visit(parser_state->rules, i);
    rules_info[i].rulelen = kev_lr_rule_get_bodylen(rule);
    rules_info[i].head_id = kev_lr_symbol_get_id(kev_lr_rule_get_head(rule));
    KevActionFunc* actfunc = kev_addrarray_visit(parser_state->redact, i);
    if (actfunc) {
      rules_info[i].actfunc = actfunc->content;
      rules_info[i].func_type = actfunc->type;
    }
  }
}

static void kev_pargen_convert_symbols(KevPTableInfo* table_info, KevPParserState* parser_state) {
  table_info->symbol_name = (char**)malloc(table_info->symbol_no * sizeof (char*));
  if (!table_info->symbol_name) {
    kev_throw_error("convert:", "out of memory", NULL);
  }
  char** symbol_name = table_info->symbol_name;
  KevStrXMap* symbol_map = parser_state->symbols;
  memset(symbol_name, 0, table_info->symbol_no * sizeof (char*));
  for (KevStrXMapNode* node = kev_strxmap_iterate_begin(symbol_map);
       node != NULL;
       node = kev_strxmap_iterate_next(symbol_map, node)) {
    KevSymbol* symbol = node->value;
    symbol_name[kev_lr_symbol_get_id(symbol)] = kev_lr_symbol_get_name(symbol);
  }
}

void kev_pargen_convert_destroy(KevPTableInfo* table_info) {
  free(table_info->action_table[0]);
  free(table_info->action_table);
  free(table_info->goto_table[0]);
  free(table_info->goto_table);
  free(table_info->symbol_name);
  free(table_info->rules_info);
  free(table_info);
}
