#include "pargen/include/lr/lr.h"
#include "pargen/include/lr/lr_print.h"
#include <stdio.h>

int main(int argc, char** argv) {
  KevSymbol* G = kev_lr_symbol_create(KEV_LR_SYMBOL_NONTERMINAL, "G");
  KevSymbol* S = kev_lr_symbol_create(KEV_LR_SYMBOL_NONTERMINAL, "S");
  KevSymbol* L = kev_lr_symbol_create(KEV_LR_SYMBOL_NONTERMINAL, "L");
  KevSymbol* R = kev_lr_symbol_create(KEV_LR_SYMBOL_NONTERMINAL, "R");
  KevSymbol* id = kev_lr_symbol_create(KEV_LR_SYMBOL_TERMINAL, "id");
  KevSymbol* star = kev_lr_symbol_create(KEV_LR_SYMBOL_TERMINAL, "*");
  KevSymbol* assign = kev_lr_symbol_create(KEV_LR_SYMBOL_TERMINAL, "=");
  KevSymbol* end = kev_lr_symbol_create(KEV_LR_SYMBOL_TERMINAL, "$");
  KevSymbol* body1[3] = { L, assign, R };
  KevSymbol* body2[2] = { star, R };
  KevRule* rule1 = kev_lr_rule_create(G, &S, 1);
  KevRule* rule2 = kev_lr_rule_create(S, &R, 1);
  KevRule* rule3 = kev_lr_rule_create(L, &id, 1);
  KevRule* rule4 = kev_lr_rule_create(R, &L, 1);
  KevRule* rule5 = kev_lr_rule_create(S, body1, 3);
  KevRule* rule6 = kev_lr_rule_create(L, body2, 2);
  KevLRCollection* collec = kev_lr_collection_create_lalr(G, &end, 1);
  kev_lr_print_collection(stdout, collec, false);
  kev_lr_collection_delete(collec);

  kev_lr_rule_delete(rule1);
  kev_lr_rule_delete(rule2);
  kev_lr_rule_delete(rule3);
  kev_lr_rule_delete(rule4);
  kev_lr_rule_delete(rule5);
  kev_lr_rule_delete(rule6);
  kev_lr_symbol_delete(L);
  kev_lr_symbol_delete(R);
  kev_lr_symbol_delete(G);
  kev_lr_symbol_delete(S);
  kev_lr_symbol_delete(id);
  kev_lr_symbol_delete(star);
  kev_lr_symbol_delete(assign);
  kev_lr_symbol_delete(end);
  return 0;
}
