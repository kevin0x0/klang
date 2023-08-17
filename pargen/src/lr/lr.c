#include "pargen/include/lr/lr.h"
#include "pargen/include/lr/item.h"
#include "utils/include/array/addr_array.h"
#include "utils/include/set/bitset.h"

#include <stdlib.h>
#include <time.h>

static bool kev_lr_closure_propagate(KevItemSet* itemset, KevAddrArray* closure, KevBitSet** la_symbols, KevBitSet** firsts, size_t epsilon);

void kev_lr_compute_first(KevBitSet** firsts, KevSymbol* symbol, size_t epsilon) {
  KevBitSet* first = firsts[symbol->tmp_id];
    for (KevRuleNode* node = symbol->rules; node; node = node->next) {
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
  KevBitSet* la = kev_lr_symbols_to_bitset(lookahead, length);
  KevItemSet* iset = kev_lr_itemset_create();
  if (!iset || !la) {
    kev_bitset_delete(la);
    kev_lr_itemset_delete(iset);
    return NULL;
  }
  for (KevRuleNode* node = start->rules; node; node = node->next) {
    KevKernelItem* item = kev_lr_kernel_item_create(node->rule, 0);
    if (!item || !(item->lookahead = kev_bitset_create_copy(la))) {
      kev_lr_itemset_delete(iset);
      kev_bitset_delete(la);
      return NULL;
    }
    kev_lr_itemset_add_item(iset, item);
  }
  kev_bitset_delete(la);
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

bool kev_lr_closure(KevItemSet* itemset, KevAddrArray* closure, KevBitSet** la_symbols, KevBitSet** firsts, size_t epsilon) {
  KevKernelItem* kitem = itemset->items;
  for (; kitem; kitem = kitem->next) {
    if (kitem->dot == kitem->rule->bodylen)
      continue;
    KevSymbol* symbol = kitem->rule->body[kitem->dot];
    if (symbol->kind == KEV_LR_SYMBOL_TERMINAL)
      continue;
    KevBitSet* la = kev_lr_get_kernel_item_follows(kitem, firsts, epsilon);
    if (!la) return false;
    size_t index = symbol->tmp_id;
    if (la_symbols[index]) {
      if (!kev_bitset_union(la_symbols[index], la)) {
        kev_bitset_delete(la);
        return false;
      }
      kev_bitset_delete(la);
    } else {
      if (!kev_addrarray_push_back(closure, symbol)) {
        kev_bitset_delete(la);
        return false;
      }
      la_symbols[index] = la;
    }
  }

  for (size_t i = 0; i < kev_addrarray_size(closure); ++i) {
    KevSymbol* head = kev_addrarray_visit(closure, i);
    size_t head_index = head->tmp_id;
    for (KevRuleNode* node = head->rules; node; node = node->next) {
      KevRule* rule = node->rule;
      if (rule->bodylen == 0) continue;
      KevSymbol* symbol = rule->body[0];
      if (symbol->kind == KEV_LR_SYMBOL_TERMINAL)
        continue;
      KevBitSet* la = kev_lr_get_non_kernel_item_follows(rule, la_symbols[head_index], firsts, epsilon);
      if (!la) return false;
      size_t index = symbol->tmp_id;
      if (la_symbols[index]) {
        if (!kev_bitset_union(la_symbols[index], la)) {
          kev_bitset_delete(la);
          return false;
        }
        kev_bitset_delete(la);
      } else {
        if (!kev_addrarray_push_back(closure, symbol)) {
          kev_bitset_delete(la);
          return false;
        }
        la_symbols[index] = la;
      }
    }
  }

  return kev_lr_closure_propagate(itemset, closure, la_symbols, firsts, epsilon);
}

bool kev_lr_closure_create(KevLRCollection* collec, KevItemSet* itemset, KevAddrArray** p_closure, KevBitSet*** p_la_symbols) {
  KevBitSet** la_symbols = (KevBitSet**)malloc(sizeof (KevBitSet*) * collec->symbol_no);
  KevAddrArray* closure = kev_addrarray_create();
  if (!la_symbols || !closure) {
    free(la_symbols);
    kev_addrarray_delete(closure);
    return false;
  }
  for (size_t i = 0; i < collec->symbol_no; ++i)
    la_symbols[i] = NULL;
  if (!kev_lr_closure(itemset, closure, la_symbols, collec->firsts, collec->terminal_no)) {
    kev_lr_closure_delete(closure, la_symbols);
    return false;
  }
  *p_closure = closure;
  *p_la_symbols = la_symbols;
  return true;
}

void kev_lr_closure_delete(KevAddrArray* closure, KevBitSet** la_symbols) {
  if (!closure || !la_symbols) return;
  kev_lr_closure_destroy(closure, la_symbols);
  free(la_symbols);
  kev_addrarray_delete(closure);
}

void kev_lr_closure_destroy(KevAddrArray* closure, KevBitSet** la_symbols) {
  size_t size = kev_addrarray_size(closure);
  for (size_t i = 0; i < size; ++i) {
    size_t index = ((KevSymbol*)kev_addrarray_visit(closure, i))->tmp_id;
    kev_bitset_delete(la_symbols[index]);
  }
}

void kev_lr_closure_make_empty(KevAddrArray* closure, KevBitSet** la_symbols) {
  size_t size = kev_addrarray_size(closure);
  for (size_t i = 0; i < size; ++i) {
    size_t index = ((KevSymbol*)kev_addrarray_visit(closure, i))->tmp_id;
    kev_bitset_delete(la_symbols[index]);
    la_symbols[index] = NULL;
  }
  kev_addrarray_make_empty(closure);
}

bool kev_lr_closure_propagate(KevItemSet* itemset, KevAddrArray* closure, KevBitSet** la_symbols, KevBitSet** firsts, size_t epsilon) {
  size_t closure_size = kev_addrarray_size(closure);
  bool propagated = true;
  while (propagated) {
    propagated = false;
    for (size_t i = 0; i < closure_size; ++i) {
      KevSymbol* symbol = (KevSymbol*)kev_addrarray_visit(closure, i);
      KevRuleNode* node = symbol->rules;
      for (; node; node = node->next) {
        size_t i = 0;
        size_t len = node->rule->bodylen;
        if (len == 0) continue;
        KevSymbol** body = node->rule->body;
        for (; i < len; ++i) {
          if (body[i]->kind == KEV_LR_SYMBOL_TERMINAL || 
              kev_bitset_has_element(la_symbols[body[i]->tmp_id], epsilon))
            break;
        }
        if (i != len) continue;
        if (!kev_bitset_is_subset(la_symbols[symbol->tmp_id], la_symbols[body[0]->tmp_id])) {
          propagated = true;
          if (!kev_bitset_union(la_symbols[body[0]->tmp_id], la_symbols[symbol->tmp_id]))
            return false;
        }
      }
    }
  }
  return true;
}

KevBitSet* kev_lr_get_kernel_item_follows(KevKernelItem* kitem, KevBitSet** firsts, size_t epsilon) {
  size_t len = kitem->rule->bodylen;
  KevSymbol** rulebody = kitem->rule->body;
  KevBitSet* follows = kev_bitset_create(epsilon + 3);
  if (!follows) return NULL;
  for (size_t i = kitem->dot + 1; i < len; ++i) {
    if (rulebody[i]->kind == KEV_LR_SYMBOL_TERMINAL) {
      kev_bitset_set(follows, rulebody[i]->tmp_id);
      return follows;
    }
    KevBitSet* set = firsts[rulebody[i]->tmp_id];
    if (!kev_bitset_union(follows, set)) {
      kev_bitset_delete(follows);
      return NULL;
    }
    if (!kev_bitset_has_element(set, epsilon)) {
      kev_bitset_clear(follows, epsilon);
      return follows;
    }
  }
  kev_bitset_clear(follows, epsilon);
  if (!kev_bitset_union(follows, kitem->lookahead)) {
    kev_bitset_delete(follows);
    return NULL;
  }
  return follows;
}

KevBitSet* kev_lr_get_non_kernel_item_follows(KevRule* rule, KevBitSet* lookahead, KevBitSet** firsts, size_t epsilon) {
  size_t len = rule->bodylen;
  KevSymbol** rulebody = rule->body;
  KevBitSet* follows = kev_bitset_create(epsilon + 3);
  if (!follows) return NULL;
  for (size_t i = 1; i < len; ++i) {
    if (rulebody[i]->kind == KEV_LR_SYMBOL_TERMINAL) {
      kev_bitset_set(follows, rulebody[i]->tmp_id);
      return follows;
    }
    KevBitSet* set = firsts[rulebody[i]->tmp_id];
    if (!kev_bitset_union(follows, set)) {
      kev_bitset_delete(follows);
      return NULL;
    }
    if (!kev_bitset_has_element(set, epsilon)) {
      kev_bitset_clear(follows, epsilon);
      return follows;
    }
  }
  kev_bitset_clear(follows, epsilon);
  if (!kev_bitset_union(follows, lookahead)) {
    kev_bitset_delete(follows);
    return NULL;
  }
  return follows;
}

void kev_lr_collection_delete(KevLRCollection* collec) {
  if (!collec) return;
  free(collec->symbols);
  for (size_t i = 0; i < collec->symbol_no; ++i) {
    kev_bitset_delete(collec->firsts[i]);
  }
  free(collec->firsts);
  for (size_t i = 0; i < collec->itemset_no; ++i) {
    kev_lr_itemset_delete(collec->itemsets[i]);
  }
  free(collec->itemsets);
  free(collec);
}
