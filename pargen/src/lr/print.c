#include "pargen/include/lr/print.h"

#define KEV_LR_SYMBOL_EPSILON_STRING  "ε"
#define KEV_UNNAMED                   "[UNNAMED]"

#include <stdio.h>

bool kev_lr_print_itemset(FILE* out, KevLRCollection* collec, KevItemSet* itemset, bool print_closure) {
  for (KevItem* kitem = itemset->items; kitem; kitem = kitem->next) {
    kev_lr_print_kernel_item(out, collec, kitem);
    fputc('\n', out);
  }
  if (!print_closure) return true;
  KevBitSet** la_symbols = NULL;
  KevAddrArray* closure = NULL;
  if (!kev_lr_closure_create(collec, itemset, &closure, &la_symbols))
    return false;
  for (size_t i = 0; i < kev_addrarray_size(closure); ++i) {
    KevSymbol* head = kev_addrarray_visit(closure, i);
    size_t head_index = head->tmp_id;
    for (KevRuleNode* node = head->rules; node; node = node->next) {
      KevRule* rule = node->rule;
      kev_lr_print_non_kernel_item(out, collec, rule, la_symbols[head_index]);
      fputc('\n', out);
    }
  }
  kev_lr_closure_delete(closure, la_symbols);
  return true;
}

bool kev_lr_print_collection(FILE* out, KevLRCollection* collec, bool print_closure) {
  for (size_t i = 0; i < collec->itemset_no; ++i) {
    fprintf(out, "item set %d:\n", (int)collec->itemsets[i]->id);
    if (!kev_lr_print_itemset(out, collec, collec->itemsets[i], print_closure))
      return false;
    fputc('\n', out);
  }
  return true;
}

void kev_lr_print_kernel_item(FILE* out, KevLRCollection* collec, KevItem* kitem) {
  KevRule* rule = kitem->rule;
  KevSymbol** body = rule->body;
  size_t len = rule->bodylen;
  fprintf(out, "%s -> ", rule->head->name ? rule->head->name : KEV_UNNAMED);
  size_t dot = kitem->dot;
  for (size_t i = 0; i < dot; ++i) 
    fprintf(out, "%s ", body[i]->name ? body[i]->name : KEV_UNNAMED);
  fprintf(out, ": ");
  for (size_t i = dot; i < len; ++i) 
    fprintf(out, "%s ", body[i]->name ? body[i]->name : KEV_UNNAMED);
  kev_lr_print_terminal_set(out, collec, kitem->lookahead);
}

void kev_lr_print_non_kernel_item(FILE* out, KevLRCollection* collec, KevRule* rule, KevBitSet* lookahead) {
  KevSymbol** body = rule->body;
  size_t len = rule->bodylen;
  fprintf(out, "%s -> ", rule->head->name ? rule->head->name : KEV_UNNAMED);
  fprintf(out, ": ");
  for (size_t i = 0; i < len; ++i) 
    fprintf(out, "%s ", body[i]->name ? body[i]->name : KEV_UNNAMED);
  kev_lr_print_terminal_set(out, collec, lookahead);
}

void kev_lr_print_terminal_set(FILE* out, KevLRCollection* collec, KevBitSet* lookahead) {
  if (!lookahead) return;
  if (kev_bitset_empty(lookahead)) {
    fprintf(out, "[]");
    return;
  }
  size_t epsilon = collec->terminal_no;
  size_t symbol_index = kev_bitset_iterate_begin(lookahead);
  size_t next_index = 0;
  char* name = collec->symbols[symbol_index]->name;
  fprintf(out, "[%s", name ? name : KEV_UNNAMED);
  next_index = kev_bitset_iterate_next(lookahead, symbol_index);
  while (next_index != symbol_index) {
    symbol_index = next_index;
    if (symbol_index != epsilon) {
      char* name = collec->symbols[symbol_index]->name;
      fprintf(out, ", %s", name ? name : KEV_UNNAMED);
    } else {
      fprintf(out, KEV_LR_SYMBOL_EPSILON_STRING);
    }
    next_index = kev_bitset_iterate_next(lookahead, symbol_index);
  }
  fputc(']', out);
}

