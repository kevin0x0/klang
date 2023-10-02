#ifndef KEVCC_PARGEN_INCLUDE_PARGEN_CONVERT_H
#define KEVCC_PARGEN_INCLUDE_PARGEN_CONVERT_H

#include "pargen/include/parser/parser.h"

typedef struct tagKevPActionEntry {
  int action;
  int info;
} KevPActionEntry;

typedef struct tagKevPRuleInfo {
  int rulelen;
  int head_id;
  char* actfunc;
  uint8_t func_type;
} KevPRuleInfo;

typedef struct tagKevPTableInfo {
  int** goto_table;
  KevPActionEntry** action_table;
  char** symbol_name;
  KevPRuleInfo* rules_info;
  int* state_to_symbol_id;
  int start_state;
  size_t action_col_no;
  size_t goto_col_no;       /* this actually is max symbol id + 1 */
  size_t terminal_no;       /* the number of terminal symbols */
  size_t state_no;
  size_t rule_no;
} KevPTableInfo;

void kev_pargen_convert(KevPTableInfo* table_info, KevLRTable* table, KevPParserState* parser_state);
void kev_pargen_convert_destroy(KevPTableInfo* table_info);

#endif
