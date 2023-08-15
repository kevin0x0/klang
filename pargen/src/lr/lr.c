#include "pargen/include/lr/lr.h"
#include "pargen/include/lr/item.h"
#include "utils/include/array/addr_array.h"

#include <stdlib.h>
#include <time.h>

static bool kev_lr_closure_propagate(KevItemSet* itemset, KevAddrArray* closure, KevBitSet** la_symbols, KevBitSet** firsts, size_t epsilon);

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

KevAddrArray* kev_lr_closure(KevLRCollection* collec, KevItemSet* itemset, KevBitSet*** p_la_symbols) {
  size_t epsilon = collec->terminal_no;
  size_t symbol_no = collec->symbol_no;
  KevBitSet** firsts = collec->firsts;
  KevBitSet** la_symbols = (KevBitSet**)malloc(sizeof (KevBitSet*) * symbol_no);
  KevAddrArray* closure = kev_addrarray_create();

  if (!la_symbols || !closure) {
    kev_addrarray_delete(closure);
    free(la_symbols);
    return NULL;
  }

  for (size_t i = 0; i < symbol_no; ++i)
    la_symbols[i] = NULL;

  bool has_la = itemset->items->lookahead != NULL;

  KevKernelItem* kitem = itemset->items;
  for (; kitem; kitem = kitem->next) {
    if (kitem->dot == kitem->rule->bodylen)
      continue;
    KevSymbol* symbol = kitem->rule->body[kitem->dot];
    if (symbol->kind == KEV_LR_SYMBOL_TERMINAL)
      continue;
    KevBitSet* la = NULL;
    if (has_la) {
      la = kev_lr_get_kernel_item_follows(collec, kitem);
      if (!la) {
        kev_addrarray_delete(closure);
        free(la_symbols);
        return NULL;
      }
    }
    size_t index = symbol->tmp_id;
    if (la_symbols[index]) {
      if (has_la) {
        if (!kev_bitset_union(la_symbols[index], la)) {
          kev_bitset_delete(la);
          kev_addrarray_delete(closure);
          free(la_symbols);
          return NULL;
        }
        kev_bitset_delete(la);
      }
    } else {
      la_symbols[index] = has_la ? la : (KevBitSet*)1;
      if (!kev_addrarray_push_back(closure, symbol)) {
        kev_addrarray_delete(closure);
        free(la_symbols);
        return NULL;
      }
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
      KevBitSet* la = NULL;
      if (has_la) {
        la = kev_lr_get_non_kernel_item_follows(collec, rule, la_symbols[head_index]);
        if (!la) {
          kev_addrarray_delete(closure);
          free(la_symbols);
          return NULL;
        }
      }
      size_t index = symbol->tmp_id;
      if (la_symbols[index]) {
        if (has_la) {
          if (!kev_bitset_union(la_symbols[index], la)) {
            kev_bitset_delete(la);
            kev_addrarray_delete(closure);
            free(la_symbols);
            return NULL;
          }
          kev_bitset_delete(la);
        }
      } else {
        la_symbols[index] = has_la ? la : (KevBitSet*)1;
        if (!kev_addrarray_push_back(closure, symbol)) {
          kev_addrarray_delete(closure);
          free(la_symbols);
          return NULL;
        }
      }
    }
  }

  if (has_la) {
    if (!kev_lr_closure_propagate(itemset, closure, la_symbols, firsts, epsilon))
      return NULL;
  }

  if (!has_la) {
    size_t size = kev_addrarray_size(closure);
    for (size_t i = 0; i < size; ++i) {
      size_t index = ((KevSymbol*)kev_addrarray_visit(closure, i))->tmp_id;
      la_symbols[index] = NULL;
    }
  }

  *p_la_symbols = la_symbols;
  return closure;
}

void kev_lr_delete_closure(KevAddrArray* closure, KevBitSet** la_symbols) {
  size_t size = kev_addrarray_size(closure);
  for (size_t i = 0; i < size; ++i) {
    size_t index = ((KevSymbol*)kev_addrarray_visit(closure, i))->tmp_id;
    kev_bitset_delete(la_symbols[index]);
  }
  free(la_symbols);
  kev_addrarray_delete(closure);
}

static bool kev_lr_closure_propagate(KevItemSet* itemset, KevAddrArray* closure, KevBitSet** la_symbols, KevBitSet** firsts, size_t epsilon) {
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

KevBitSet* kev_lr_get_kernel_item_follows(KevLRCollection* collec, KevKernelItem* kitem) {
  size_t epsilon = collec->terminal_no;
  KevBitSet** firsts = collec->firsts;

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

KevBitSet* kev_lr_get_non_kernel_item_follows(KevLRCollection* collec, KevRule* rule, KevBitSet* lookahead) {
  size_t epsilon = collec->terminal_no;
  KevBitSet** firsts = collec->firsts;

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

void kev_lr_delete_collection(KevLRCollection* collec) {
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
