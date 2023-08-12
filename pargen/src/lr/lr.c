#include "pargen/include/lr/lr.h"

#include <stdlib.h>

void kev_lr_compute_first(KevBitSet** firsts, KevSymbol* symbol, size_t epsilon) {
  KevBitSet* first = firsts[symbol->tmp_id];
    KevRuleNode* node = symbol->rules;
    while (node) {
      KevRule* rule = node->rule;
      size_t bodylen = rule->bodylen;
      KevSymbol** body = rule->body;
      size_t i = 0;
      for (; i < bodylen; ++i) {
        if (body[i]->kind == KEV_LR_SYMBOL_TERMINAL) {
          kev_bitset_set(first, body[i]->tmp_id);
          break;
        }
        KevBitSet* curr = firsts[body[i]->tmp_id];
        /* all first sets has same size, so union will never fail */
        kev_bitset_union(first, curr);
        if (!kev_bitset_has_element(curr, epsilon))
          break;
      }
      if (i != bodylen)
        kev_bitset_clear(first, epsilon);
      else
        kev_bitset_set(first, epsilon);
      node = node->next;
    }
}

KevBitSet** kev_lr_compute_first_array(KevSymbol** symbols, size_t symbol_no, size_t terminal_no) {
  KevBitSet** firsts = (KevBitSet**)malloc(sizeof (KevBitSet*) * symbol_no);
  if (!firsts) return NULL;
  KevBitSet backup;
  if (!kev_bitset_init(&backup, terminal_no + 3)) {
    free (firsts);
    return NULL;
  }
  /* initialize */
  for (size_t i = 0; i < terminal_no; ++i)
    firsts[i] = NULL;
  for (size_t i = terminal_no; i < symbol_no; ++i) {
    if (!(firsts[i] = kev_bitset_create(terminal_no + 3))) {
      for (size_t j = terminal_no; j < i; ++j)
        kev_bitset_delete(firsts[j]);
      free(firsts);
      kev_bitset_destroy(&backup);
      return NULL;
    }
  }
  /* compute firsts */
  bool loop = true;
  while (loop) {
    loop = false;
    for (size_t i = terminal_no; i < symbol_no; ++i) {
      if (!loop) kev_bitset_assign(&backup, firsts[i]);
      kev_lr_compute_first(firsts, symbols[i], terminal_no);
      if (!loop && !kev_bitset_equal(&backup, firsts[i]))
        loop = true;
    }
  }
  kev_bitset_destroy(&backup);
  return firsts;
}

KevSymbol* kev_lr_augment(KevSymbol* start) {
  KevSymbol* new_start = kev_lr_symbol_create(KEV_LR_SYMBOL_NONTERMINAL, NULL, 0);
  if (!new_start) return NULL;
  KevRule* start_rule = kev_lr_rule_create(new_start, &start, 1);
  if (!start_rule) {
    kev_lr_symbol_delete(new_start);
    return NULL;
  }
  return new_start;
}

void kev_lr_destroy_first_array(KevBitSet** firsts, size_t size) {
  for (size_t i = 0; i < size; ++i)
    kev_bitset_delete(firsts[i]);
  free(firsts);
}

KevBitSet* kev_lr_symbols_to_bitset(KevSymbol** symbols, size_t length) {
  KevBitSet* set = kev_bitset_create(1);
  if (!set) return NULL;
  for (size_t i = 0; i < length; ++i) {
    if (!kev_bitset_set(set, symbols[i]->tmp_id)) {
      kev_bitset_delete(set);
      return NULL;
    }
  }
  return set;
}

KevItemSet* kev_lr_get_start_itemset(KevSymbol* start, KevSymbol** lookahead, size_t length) {
  KevBitSet* la_bitset = kev_lr_symbols_to_bitset(lookahead, length);
  KevItemSet* iset = kev_lr_itemset_create();
  if (!iset || !la_bitset) {
    kev_bitset_delete(la_bitset);
    kev_lr_itemset_delete(iset);
    return NULL;
  }
  KevRuleNode* node = start->rules;
  while (node) {
    KevKernelItem* item = kev_lr_kernel_item_create(node->rule, 0);
    if (!item || !(item->lookahead = kev_bitset_create_copy(la_bitset))) {
      kev_lr_itemset_delete(iset);
      kev_bitset_delete(la_bitset);
      return NULL;
    }
    kev_lr_itemset_add_item(iset, item);
    node = node->next;
  }
  kev_bitset_delete(la_bitset);
  return iset;
}

size_t kev_lr_label_symbols(KevSymbol** symbols, size_t symbol_no) {
  size_t number = 0;
  size_t i = 0;
  while (symbols[i]->kind == KEV_LR_SYMBOL_TERMINAL) {
    symbols[i]->tmp_id = i;
    ++i;
  }
  number = i;
  for (; i < symbol_no; ++i)
    symbols[i]->tmp_id = i;
  return number;
}
