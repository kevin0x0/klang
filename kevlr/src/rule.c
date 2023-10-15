#include "kevlr/include/rule.h"
#include "utils/include/string/kev_string.h"

#include <stdlib.h>

static inline void kev_rulenode_delete(KlrRuleNode* rules);

KlrSymbol* klr_symbol_create(KlrSymbolKind kind, const char* name) {
  KlrSymbol* symbol = (KlrSymbol*)malloc(sizeof (KlrSymbol));
  if (!symbol) return NULL;
  symbol->kind = kind;
  symbol->name = NULL;
  if (name && !(symbol->name = kev_str_copy(name))) {
    free(symbol);
    return NULL;
  }
  symbol->rules = NULL;
  return symbol;
}

KlrSymbol* klr_symbol_create_move(KlrSymbolKind kind, char* name) {
  KlrSymbol* symbol = (KlrSymbol*)malloc(sizeof (KlrSymbol));
  if (!symbol) return NULL;
  symbol->kind = kind;
  symbol->name = name;
  symbol->rules = NULL;
  return symbol;
}

void klr_symbol_delete(KlrSymbol* symbol) {
  if (symbol) {
    kev_rulenode_delete(symbol->rules);
    free(symbol->name);
    free(symbol);
  }
}

KlrRule* klr_rule_create(KlrSymbol* head, KlrSymbol** body, size_t body_length) {
  KlrRule* rule = (KlrRule*)malloc(sizeof (KlrRule));
  KlrSymbol** rule_body = (KlrSymbol**)malloc(sizeof (KlrSymbol*) * body_length);
  KlrRuleNode* rulenode = (KlrRuleNode*)malloc(sizeof (KlrRuleNode));
  if (!rule || !rule_body || !rulenode) {
    free(rule);
    free(rule_body);
    free(rulenode);
    return NULL;
  }
  rulenode->rule = rule;
  rulenode->next = head->rules;
  head->rules = rulenode;
  rule->head = head;
  rule->body = rule_body;
  rule->bodylen = body_length;
  for (size_t i = 0; i < body_length; ++i)
    rule_body[i] = body[i];
  return rule;
}

KlrRule* klr_rule_create_move(KlrSymbol* head, KlrSymbol** body, size_t body_length) {
  KlrRule* rule = (KlrRule*)malloc(sizeof (KlrRule));
  KlrRuleNode* rulenode = (KlrRuleNode*)malloc(sizeof (KlrRuleNode));
  if (!rule || !rulenode) return NULL;
  rulenode->rule = rule;
  rulenode->next = head->rules;
  head->rules = rulenode;
  rule->head = head;
  rule->body = body;
  rule->bodylen = body_length;
  return rule;
}

void klr_rule_delete(KlrRule* rule) {
  if (rule) {
    free(rule->body);
    free(rule);
  }
}

static inline void kev_rulenode_delete(KlrRuleNode* rules) {
  while (rules) {
    KlrRuleNode* tmp = rules->next;
    free(rules);
    rules = tmp;
  }
}

bool klr_symbol_set_name(KlrSymbol* symbol, const char* name) {
  free(symbol->name);
  symbol->name = NULL;
  return name && !(symbol->name = kev_str_copy(name));
}

void klr_symbol_set_name_move(KlrSymbol* symbol, char* name) {
  free(symbol->name);
  symbol->name = name;
}
