#include "kevlr/include/rule.h"
#include "utils/include/string/kev_string.h"

#include <stdlib.h>

static inline void kev_rulenode_delete(KevRuleNode* rules);

KevSymbol* kev_lr_symbol_create(KevSymbolType kind, const char* name) {
  KevSymbol* symbol = (KevSymbol*)malloc(sizeof (KevSymbol));
  if (!symbol) return NULL;
  symbol->kind = kind;
  symbol->name = kev_str_copy(name);
  symbol->rules = NULL;
  return symbol;
}

KevSymbol* kev_lr_symbol_create_move(KevSymbolType kind, char* name) {
  KevSymbol* symbol = (KevSymbol*)malloc(sizeof (KevSymbol));
  if (!symbol) return NULL;
  symbol->kind = kind;
  symbol->name = name;
  symbol->rules = NULL;
  return symbol;
}

void kev_lr_symbol_delete(KevSymbol* symbol) {
  if (symbol) {
    kev_rulenode_delete(symbol->rules);
    free(symbol->name);
    free(symbol);
  }
}

KevRule* kev_lr_rule_create(KevSymbol* head, KevSymbol** body, size_t body_length) {
  KevRule* rule = (KevRule*)malloc(sizeof (KevRule));
  KevSymbol** rule_body = (KevSymbol**)malloc(sizeof (KevSymbol*) * body_length);
  KevRuleNode* rulenode = (KevRuleNode*)malloc(sizeof (KevRuleNode));
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

KevRule* kev_lr_rule_create_move(KevSymbol* head, KevSymbol** body, size_t body_length) {
  KevRule* rule = (KevRule*)malloc(sizeof (KevRule));
  KevRuleNode* rulenode = (KevRuleNode*)malloc(sizeof (KevRuleNode));
  if (!rule || !rulenode) return NULL;
  rulenode->rule = rule;
  rulenode->next = head->rules;
  head->rules = rulenode;
  rule->head = head;
  rule->body = body;
  rule->bodylen = body_length;
  return rule;
}

void kev_lr_rule_delete(KevRule* rule) {
  if (rule) {
    free(rule->body);
    free(rule);
  }
}

static inline void kev_rulenode_delete(KevRuleNode* rules) {
  while (rules) {
    KevRuleNode* tmp = rules->next;
    free(rules);
    rules = tmp;
  }
}
