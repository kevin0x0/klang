#ifndef KEVCC_KEVLR_INCLUDE_RULE_H
#define KEVCC_KEVLR_INCLUDE_RULE_H

#include "utils/include/general/global_def.h"

#define KLR_TERMINAL     ((KlrSymbolKind)0)
#define KLR_NONTERMINAL  ((KlrSymbolKind)1)

typedef size_t KlrID;
typedef uint8_t KlrSymbolKind;
struct tagKlrRule;
struct tagKlrRuleNode;

typedef struct tagKlrSymbol {
  char* name;
  KlrID id;
  size_t index;
  KlrSymbolKind kind;
  struct tagKlrRuleNode* rules;
} KlrSymbol;

typedef struct tagKlrRule {
  KlrSymbol* head;
  KlrSymbol** body;
  size_t bodylen;
  size_t id;
} KlrRule;

typedef struct tagKlrRuleNode {
  KlrRule* rule;
  struct tagKlrRuleNode* next;
} KlrRuleNode;

KlrSymbol* klr_symbol_create(KlrSymbolKind kind, const char* name);
KlrSymbol* klr_symbol_create_move(KlrSymbolKind kind, char* name);
void klr_symbol_delete(KlrSymbol* symbol);
KlrRule* klr_rule_create(KlrSymbol* head, KlrSymbol** body, size_t body_length);
KlrRule* klr_rule_create_move(KlrSymbol* head, KlrSymbol** body, size_t body_length);
void klr_rule_delete(KlrRule* rule);

/* get method */
static inline KlrSymbolKind klr_symbol_get_kind(KlrSymbol* symbol);
static inline char* klr_symbol_get_name(KlrSymbol* symbol);
static inline KlrID klr_symbol_get_id(KlrSymbol* symbol);

static inline KlrSymbol* klr_rule_get_head(KlrRule* rule);
static inline KlrSymbol** klr_rule_get_body(KlrRule* rule);
static inline size_t klr_rule_get_bodylen(KlrRule* rule);
static inline KlrID klr_rule_get_id(KlrRule* rule);

/* set method */
bool klr_symbol_set_name(KlrSymbol* symbol, const char* name);
void klr_symbol_set_name_move(KlrSymbol* symbol, char* name);
static inline void klr_symbol_set_id(KlrSymbol* symbol, KlrID id);
static inline void klr_rule_set_id(KlrRule* rule, KlrID id);

static inline KlrSymbolKind klr_symbol_get_kind(KlrSymbol* symbol) {
  return symbol->kind;
}

static inline char* klr_symbol_get_name(KlrSymbol* symbol) {
  return symbol->name;
}

static inline KlrID klr_symbol_get_id(KlrSymbol* symbol) {
  return symbol->id;
}

static inline void klr_symbol_set_id(KlrSymbol* symbol, KlrID id) {
  symbol->id = id;
}

static inline KlrSymbol* klr_rule_get_head(KlrRule* rule) {
  return rule->head;
}

static inline KlrSymbol** klr_rule_get_body(KlrRule* rule) {
  return rule->body;
}

static inline size_t klr_rule_get_bodylen(KlrRule* rule) {
  return rule->bodylen;
}

static inline KlrID klr_rule_get_id(KlrRule* rule) {
  return rule->id;
}

static inline void klr_rule_set_id(KlrRule* rule, KlrID id) {
  rule->id = id;
}

#endif
