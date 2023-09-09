#ifndef KEVCC_KEVLR_INCLUDE_RULE_H
#define KEVCC_KEVLR_INCLUDE_RULE_H

#include "utils/include/general/global_def.h"

#define KEV_LR_TERMINAL     (0)
#define KEV_LR_NONTERMINAL  (1)

typedef size_t KevSymbolID;
struct tagKevRule;
struct tagKevRuleNode;

typedef struct tagKevSymbol {
  char* name;
  KevSymbolID id;
  size_t index;
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

KevSymbol* kev_lr_symbol_create(int kind, const char* name);
KevSymbol* kev_lr_symbol_create_move(int kind, char* name);
void kev_lr_symbol_delete(KevSymbol* symbol);
KevRule* kev_lr_rule_create(KevSymbol* head, KevSymbol** body, size_t body_length);
KevRule* kev_lr_rule_create_move(KevSymbol* head, KevSymbol** body, size_t body_length);
void kev_lr_rule_delete(KevRule* rule);

/* get method */
static inline int kev_lr_symbol_get_type(KevSymbol* symbol);
static inline char* kev_lr_symbol_get_name(KevSymbol* symbol);
static inline KevSymbolID kev_lr_symbol_get_id(KevSymbol* symbol);

static inline KevSymbol* kev_lr_rule_get_head(KevRule* rule);
static inline KevSymbol** kev_lr_rule_get_body(KevRule* rule);
static inline size_t kev_lr_rule_get_bodylen(KevRule* rule);

/* set method */
static inline void kev_lr_symbol_set_id(KevSymbol* symbol, KevSymbolID id);

static inline int kev_lr_symbol_get_type(KevSymbol* symbol) {
  return symbol->kind;
}

static inline char* kev_lr_symbol_get_name(KevSymbol* symbol) {
  return symbol->name;
}

static inline KevSymbolID kev_lr_symbol_get_id(KevSymbol* symbol) {
  return symbol->id;
}

static inline void kev_lr_symbol_set_id(KevSymbol* symbol, KevSymbolID id) {
  symbol->id = id;
}

static inline KevSymbol* kev_lr_rule_get_head(KevRule* rule) {
  return rule->head;
}

static inline KevSymbol** kev_lr_rule_get_body(KevRule* rule) {
  return rule->body;
}

static inline size_t kev_lr_rule_get_bodylen(KevRule* rule) {
  return rule->bodylen;
}

#endif
