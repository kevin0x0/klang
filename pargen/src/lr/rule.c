#include "pargen/include/lr/rule.h"
#include "utils/include/array/addr_array.h"
#include "utils/include/set/hashset.h"

#include <stdlib.h>

static inline void kev_rulenode_delete(KevRuleNode* rules);

KevSymbol* kev_lr_symbol_create(int kind, char* name) {
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
    free(symbol);
  }
}

KevRule* kev_lr_rule_create(KevSymbol* head, KevSymbol** body, size_t body_length) {
  if (!head || !body) return NULL;
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
  if (!head || !body) return NULL;
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

KevSymbol** kev_lr_get_symbol_array(KevSymbol* start, KevSymbol** lookahead, size_t la_len, size_t* p_size) {
  KevAddrArray array;
  if (!kev_addrarray_init(&array))
    return NULL;

  KevHashSet set;
  if (!kev_hashset_init(&set, 64)) {
    kev_hashset_destroy(&set);
    return NULL;
  }
  if (!kev_hashset_insert(&set, start) ||
      !kev_addrarray_push_back(&array, start)) {
    kev_hashset_destroy(&set);
    kev_addrarray_destroy(&array);
    return NULL;
  }
  size_t curr = 0;

  while (curr != kev_addrarray_size(&array)) {
    KevSymbol* symbol = kev_addrarray_visit(&array, curr++);
    KevRuleNode* rule = symbol->rules;
    for (; rule; rule = rule->next) {
      KevSymbol** rule_body = rule->rule->body;
      size_t len = rule->rule->bodylen;
      for (size_t i = 0; i < len; ++i) {
        if (kev_hashset_has(&set, rule_body[i]))
          continue;
        if (!kev_hashset_insert(&set, rule_body[i]) ||
            !kev_addrarray_push_back(&array, rule_body[i])) {
          kev_hashset_destroy(&set);
          kev_addrarray_destroy(&array);
          return NULL;
        }
      }
    }
  }

  for (size_t i = 0; i < la_len; ++i) {
    if (kev_hashset_has(&set, lookahead[i]))
      continue;
    if (lookahead[i]->kind == KEV_LR_SYMBOL_NONTERMINAL) {
      kev_hashset_destroy(&set);
      kev_addrarray_destroy(&array);
      return NULL;
    }
    if (!kev_hashset_insert(&set, lookahead[i]) ||
        !kev_addrarray_push_back(&array, lookahead[i])) {
      kev_hashset_destroy(&set);
      kev_addrarray_destroy(&array);
      return NULL;
    }
  }

  kev_hashset_destroy(&set);
  *p_size = kev_addrarray_size(&array);
  return (KevSymbol**)array.begin;
}

void kev_lr_symbol_array_partition(KevSymbol** array, size_t size) {
  KevSymbol** left = array;
  KevSymbol** right = array + size - 1;
  while (left < right) {
    while (left < right && (*left)->kind == KEV_LR_SYMBOL_TERMINAL)
      ++left;
    while (left < right && (*right)->kind == KEV_LR_SYMBOL_NONTERMINAL)
      --right;
    if (left >= right) break;
    KevSymbol* tmp = *left;
    *left = *right;
    *right = tmp;
  }
}