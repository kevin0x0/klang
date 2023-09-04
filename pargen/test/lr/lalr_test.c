#include "pargen/include/lr/lr.h"
#include "pargen/include/lr/lr_utils.h"
#include <stdio.h>
#include <stdlib.h>

bool conflict_report(void* object, KevLRConflict* conflict, KevLRCollection* collec) {
  printf("All conflict items here:\n");
  kev_lr_print_itemset(stdout, collec, conflict->conflct_items, false);
  return false;
}

int main(int argc, char** argv) {
  KevSymbol* S = kev_lr_symbol_create(KEV_LR_SYMBOL_NONTERMINAL, "S");
  KevSymbol* L = kev_lr_symbol_create(KEV_LR_SYMBOL_NONTERMINAL, "L");
  KevSymbol* R = kev_lr_symbol_create(KEV_LR_SYMBOL_NONTERMINAL, "R");
  KevSymbol* id = kev_lr_symbol_create(KEV_LR_SYMBOL_TERMINAL, "id");
  KevSymbol* star = kev_lr_symbol_create(KEV_LR_SYMBOL_TERMINAL, "*");
  KevSymbol* assign = kev_lr_symbol_create(KEV_LR_SYMBOL_TERMINAL, "=");
  KevSymbol* end = kev_lr_symbol_create(KEV_LR_SYMBOL_TERMINAL, "$");
  KevSymbol* body1[3] = { L, assign, R };
  KevSymbol* body2[2] = { star, R };
  KevRule* rule1 = kev_lr_rule_create(S, &R, 1);
  KevRule* rule2 = kev_lr_rule_create(L, &id, 1);
  KevRule* rule3 = kev_lr_rule_create(R, &L, 1);
  KevRule* rule4 = kev_lr_rule_create(S, body1, 3);
  KevRule* rule5 = kev_lr_rule_create(L, body2, 2);
  KevRule* rule6 = kev_lr_rule_create(R, NULL, 0);
  KevLRCollection* collec = kev_lr_collection_create_slr(S, &end, 1);

  KevBitSet** follows = kev_lr_util_compute_follows(collec->symbols, collec->firsts, collec->symbol_no, collec->terminal_no, collec->start, &end, 1);

  for (size_t i = collec->terminal_no; i < collec->symbol_no; ++i) {
    fprintf(stdout, "%s ", collec->symbols[i]->name);
    kev_lr_print_terminal_set(stdout, collec, collec->firsts[i]);
    fputc('\n', stdout);
  }
  for (size_t i = collec->terminal_no; i < collec->symbol_no; ++i) {
    kev_bitset_delete(follows[i]);
  }
  free(follows);

  kev_lr_print_collection(stdout, collec, true);

  S->id = 0;
  L->id = 1;
  R->id = 2;
  id->id = 3;
  star->id = 4;
  assign->id = 5;
  end->id = 6;

  rule1->id = 0;
  rule2->id = 1;
  rule3->id = 2;
  rule4->id = 3;
  rule5->id = 4;
  rule6->id = 5;

  KevLRConflictHandler* handler = kev_lr_conflict_handler_create(NULL, conflict_report);
  KevLRTable* table = kev_lr_table_create(collec, handler);
  fputc('\n', stdout);
  fputc('\n', stdout);
  fputc('\n', stdout);
  kev_lr_print_symbols(stdout, collec);
  fputc('\n', stdout);
  fprintf(stdout, "%d ", 0);
  kev_lr_print_rule(stdout, rule1);
  fputc('\n', stdout);
  fprintf(stdout, "%d ", 1);
  kev_lr_print_rule(stdout, rule2);
  fputc('\n', stdout);
  fprintf(stdout, "%d ", 2);
  kev_lr_print_rule(stdout, rule3);
  fputc('\n', stdout);
  fprintf(stdout, "%d ", 3);
  kev_lr_print_rule(stdout, rule4);
  fputc('\n', stdout);
  fprintf(stdout, "%d ", 4);
  kev_lr_print_rule(stdout, rule5);
  fputc('\n', stdout);
  fputc('\n', stdout);
  kev_lr_print_goto_table(stdout, table);
  fputc('\n', stdout);
  kev_lr_print_action_table(stdout, table);
  kev_lr_conflict_handler_delete(handler);
  kev_lr_collection_delete(collec);
  kev_lr_table_delete(table);

  kev_lr_rule_delete(rule1);
  kev_lr_rule_delete(rule2);
  kev_lr_rule_delete(rule3);
  kev_lr_rule_delete(rule4);
  kev_lr_rule_delete(rule5);
  kev_lr_rule_delete(rule6);
  kev_lr_symbol_delete(L);
  kev_lr_symbol_delete(R);
  kev_lr_symbol_delete(S);
  kev_lr_symbol_delete(id);
  kev_lr_symbol_delete(star);
  kev_lr_symbol_delete(assign);
  kev_lr_symbol_delete(end);
  return 0;
}
