#include "pargen/include/lr/lr.h"
#include "pargen/include/lr/rule.h"

int main(int argc, char** argv) {
  KevSymbol* E = kev_lr_symbol_create(KEV_LR_SYMBOL_NONTERMINAL, "expression", 0);
  KevSymbol* G = kev_lr_symbol_create(KEV_LR_SYMBOL_NONTERMINAL, "augmented head", 1);
  KevSymbol* id = kev_lr_symbol_create(KEV_LR_SYMBOL_TERMINAL, "identifier", 2);
  KevSymbol* plus = kev_lr_symbol_create(KEV_LR_SYMBOL_TERMINAL, "plus", 3);
  KevSymbol* end = kev_lr_symbol_create(KEV_LR_SYMBOL_TERMINAL, "end", 4);
  KevSymbol* expr[3] = { E, plus, E };
  KevRule* rule1 = kev_lr_rule_create(E, expr, 3);
  KevRule* rule2 = kev_lr_rule_create(E, &id, 1);
  KevRule* rule3 = kev_lr_rule_create(G, &E, 1);
  KevLRCollection* collec = kev_lalr_generate(G, &end, 1);
  kev_lr_rule_delete(rule1);
  kev_lr_rule_delete(rule2);
  kev_lr_rule_delete(rule3);
  kev_lr_symbol_delete(E);
  kev_lr_symbol_delete(G);
  kev_lr_symbol_delete(id);
  kev_lr_symbol_delete(plus);
  return 0;
}
