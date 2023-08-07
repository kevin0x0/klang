#ifndef KEVCC_PARGEN_INCLUDE_LR_RULE_H
#define KEVCC_PARGEN_INCLUDE_LR_RULE_H

#include "utils/include/general/global_def.h"

#define KEV_LR_SYMBOL_TERMINAL    (0)
#define KEV_LR_SYMBOL_NONTERMINAL (1)

typedef int64_t KevSymbolID;
struct tagKevRule;
struct tagKevRuleNode;

typedef struct tagKevSymbol {
  char* name;
  KevSymbolID id;
  KevSymbolID tmp_id;
  int kind;
  struct tagKevRuleNode* rules;
} KevSymbol;

typedef struct tagKevRule {
  KevSymbol* head;
  KevSymbol** body;
  size_t bodylen;
  size_t id;
} KevRule;
typedef struct tagKevRuleNode {
  KevRule* rule;
  struct tagKevRuleNode* next;
} KevRuleNode;

KevSymbol* kev_lr_symbol_create(int kind, char* name, KevSymbolID id);
void kev_lr_symbol_delete(KevSymbol* symbol);
KevRule* kev_lr_rule_create(KevSymbol* head, KevSymbol** body, size_t body_length);
KevRule* kev_lr_rule_create_move(KevSymbol* head, KevSymbol** body, size_t body_length);
void kev_lr_rule_delete(KevRule* rule);

KevSymbol** kev_lr_get_symbol_array(KevSymbol* start, size_t* p_size);
void kev_lr_symbol_array_partition(KevSymbol** array, size_t size);

#endif
