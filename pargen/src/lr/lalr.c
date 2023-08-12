#include "pargen/include/lr/lalr.h"
#include "pargen/include/lr/item.h"
#include "pargen/include/lr/hashmap/gotomap.h"
#include "pargen/include/lr/set/itemset_set.h"
#include "utils/include/array/addr_array.h"

#include <stdlib.h>

static bool kev_lalr_get_all_itemsets(KevItemSet* start_iset, KevLALRCollection* collec);
static bool kev_lalr_do_lookahead_propagation(KevLookaheadPropagation* propagation);
static bool kev_lalr_merge_gotos(KevItemSetSet* iset_set, KevAddrArray* itemset_array, KevLALRCollection* collec, KevItemSet* itemset);
static bool kev_lalr_merge_itemset(KevItemSet* new, KevItemSet* old, KevItemSet* itemset, KevLALRCollection* collec);
static bool kev_lalr_add_new_itemset(KevItemSet* new, KevItemSetSet* iset_set, KevItemSet* itemset, KevLALRCollection* collec);
static bool kev_lalr_get_itemset(KevItemSet* itemset, KevAddrArray* closure, KevBitSet** la_symbols, KevBitSet** firsts, size_t epsilon, KevGotoMap* goto_container);
static bool kev_lalr_generate_gotos(KevItemSet* itemset, KevAddrArray* closure, KevBitSet** la_symbols, KevGotoMap* goto_container);
static void kev_lalr_clear_la_symbols_and_closure(KevAddrArray* closure, KevBitSet** la_symbols);
static bool kev_lalr_closure(KevItemSet* itemset, KevAddrArray* closure, KevBitSet** la_symbols, KevBitSet** firsts, size_t epsilon);
static bool kev_lalr_closure_propagate(KevItemSet* itemset, KevAddrArray* closure, KevBitSet** la_symbols, KevBitSet** firsts, size_t epsilon);
/* initialize lookahead for kernel items in itemset */
static inline bool kev_lalr_init_kitem_la(KevItemSet* itemset, size_t epsilon);
static KevBitSet* kev_lalr_get_follows(KevKernelItem* kitem, KevBitSet** firsts, size_t epsilon);

static KevLALRCollection* kev_lalr_get_empty_collec(void);
static bool kev_lalr_itemset_equal(KevItemSet* itemset1, KevItemSet* itemset2);
static inline KevLookaheadPropagation* kev_lalr_propagation_create(KevBitSet* from, KevBitSet* to);
static KevLRCollection* kev_lalr_to_lr_collec(KevLALRCollection* lalr_collec);

static void kev_lalr_destroy_itemset_array(KevAddrArray* itemset_array);
static void kev_lalr_destroy_collec(KevLALRCollection* collec);

KevLRCollection* kev_lalr_generate(KevSymbol* start, KevSymbol** lookahead, size_t la_len) {
  KevLALRCollection* collec = kev_lalr_get_empty_collec();
  if (!collec) return NULL;
  collec->symbols = kev_lr_get_symbol_array(start, lookahead, la_len, &collec->symbol_no);
  if (!collec->symbols) {
    kev_lalr_destroy_collec(collec);
    return NULL;
  }
  kev_lr_symbol_array_partition(collec->symbols, collec->symbol_no);
  collec->terminal_no = kev_lr_label_symbols(collec->symbols, collec->symbol_no);
  collec->firsts = kev_lr_compute_first_array(collec->symbols, collec->symbol_no, collec->terminal_no);
  if (!collec->firsts) {
    kev_lalr_destroy_collec(collec);
    return NULL;
  }
  KevItemSet* start_iset = kev_lr_get_start_itemset(start, lookahead, la_len);
  if (!start_iset) {
    kev_lalr_destroy_collec(collec);
    return NULL;
  }
  if (!kev_lalr_get_all_itemsets(start_iset, collec)) {
    kev_lalr_destroy_collec(collec);
    return NULL;
  }
  if (!kev_lalr_do_lookahead_propagation(collec->propagation)) {
    kev_lalr_destroy_collec(collec);
    return NULL;
  }
  KevLRCollection* lr_collec = kev_lalr_to_lr_collec(collec);
  if (!lr_collec) kev_lalr_destroy_collec(collec);
  return lr_collec;
}

static KevLALRCollection* kev_lalr_get_empty_collec(void) {
  KevLALRCollection* collec = (KevLALRCollection*)malloc(sizeof (KevLALRCollection));
  if (!collec) return NULL;
  collec->firsts = NULL;
  collec->symbols = NULL;
  collec->itemsets = NULL;
  collec->propagation = NULL;
  return collec;
}

static void kev_lalr_destroy_collec(KevLALRCollection* collec) {
  if (collec->firsts)
    kev_lr_destroy_first_array(collec->firsts, collec->symbol_no);
  if (collec->itemsets) {
    for (size_t i = 0; i < collec->itemset_no; ++i) {
      kev_lr_itemset_delete(collec->itemsets[i]);
    }
    free(collec->itemsets);
  }
  if (collec->symbols)
    free(collec->symbols);
  if (collec->propagation) {
    KevLookaheadPropagation* prop = collec->propagation;
    while (prop) {
      KevLookaheadPropagation* tmp = prop->next;
      free(prop);
      prop = tmp;
    }
  }
  free(collec);
}

static bool kev_lalr_get_all_itemsets(KevItemSet* start_iset, KevLALRCollection* collec) {
  KevAddrArray* itemset_array = kev_addrarray_create();
  KevItemSetSet* iset_set = kev_itemsetset_create(16, kev_lalr_itemset_equal);
  KevAddrArray* closure = kev_addrarray_create();
  KevBitSet** la_symbols = (KevBitSet**)malloc(sizeof (KevBitSet*) * collec->symbol_no);
  KevGotoMap* goto_container = kev_gotomap_create(16);
  if (!itemset_array || !closure || !la_symbols || !goto_container || !iset_set) {
    kev_addrarray_delete(itemset_array);
    kev_addrarray_delete(closure);
    kev_gotomap_delete(goto_container);
    kev_itemsetset_delete(iset_set);
    free(la_symbols);
    return false;
  }

  for (size_t i = 0; i < collec->symbol_no; ++i)
    la_symbols[i] = NULL;
  if (!kev_addrarray_push_back(itemset_array, start_iset) || !kev_itemsetset_insert(iset_set, start_iset)) {
    kev_lalr_destroy_itemset_array(itemset_array);
    kev_addrarray_delete(closure);
    kev_gotomap_delete(goto_container);
    kev_itemsetset_delete(iset_set);
    free(la_symbols);
    return false;
  }

  KevBitSet** firsts = collec->firsts;
  size_t terminal_no = collec->terminal_no;
  for (size_t i = 0; i < kev_addrarray_size(itemset_array); ++i) {
    KevItemSet* itemset = kev_addrarray_visit(itemset_array, i);
    if (!kev_lalr_get_itemset(itemset, closure, la_symbols, firsts, terminal_no, goto_container)) {
      kev_lalr_destroy_itemset_array(itemset_array);
      kev_addrarray_delete(closure);
      kev_gotomap_delete(goto_container);
      kev_itemsetset_delete(iset_set);
      free(la_symbols);
      return false;
    }
    if (!kev_lalr_merge_gotos(iset_set, itemset_array, collec, itemset)) {
      kev_lalr_destroy_itemset_array(itemset_array);
      kev_addrarray_delete(closure);
      kev_gotomap_delete(goto_container);
      kev_itemsetset_delete(iset_set);
      free(la_symbols);
      return false;
    }
    kev_lalr_clear_la_symbols_and_closure(closure, la_symbols);
  }
  kev_addrarray_delete(closure);
  kev_gotomap_delete(goto_container);
  kev_itemsetset_delete(iset_set);
  free(la_symbols);
  collec->itemsets = (KevItemSet**)itemset_array->begin;
  collec->itemset_no = kev_addrarray_size(itemset_array);
  return true;
}

static void kev_lalr_destroy_itemset_array(KevAddrArray* itemset_array) {
  for (size_t i = 0; i < kev_addrarray_size(itemset_array); ++i) {
    kev_lr_itemset_delete(*(KevItemSet**)kev_addrarray_visit(itemset_array, i));
  }
  kev_addrarray_delete(itemset_array);
}

static bool kev_lalr_merge_gotos(KevItemSetSet* iset_set, KevAddrArray* itemset_array, KevLALRCollection* collec, KevItemSet* itemset) {
  KevItemSetGoto* gotos = itemset->gotos;
  for (; gotos; gotos = gotos->next) {
    KevItemSet* target = gotos->itemset;
    KevItemSetSetNode* node = kev_itemsetset_search(iset_set, target);
    if (node) {
      if (!kev_lalr_merge_itemset(target, node->element, itemset, collec)) {
        for (; gotos; gotos = gotos->next) {
          kev_lr_itemset_delete(gotos->itemset);
          gotos->itemset = NULL;
        }
        return false;
      }
      kev_lr_itemset_delete(target);
      gotos->itemset = node->element;
    } else {
      if (!kev_lalr_add_new_itemset(target, iset_set, itemset, collec)) {
        for (; gotos; gotos = gotos->next) {
          kev_lr_itemset_delete(gotos->itemset);
          gotos->itemset = NULL;
        }
        return false;
      }
    }
  }
  return true;
}

static bool kev_lalr_get_itemset(KevItemSet* itemset, KevAddrArray* closure, KevBitSet** la_symbols, KevBitSet** firsts, size_t epsilon, KevGotoMap* goto_container) {
  if (!kev_lalr_closure(itemset, closure, la_symbols, firsts, epsilon))
    return false;
  if (!kev_lalr_generate_gotos(itemset, closure, la_symbols, goto_container))
    return false;
  return true;
}

static bool kev_lalr_generate_gotos(KevItemSet* itemset, KevAddrArray* closure, KevBitSet** la_symbols, KevGotoMap* goto_container) {
  kev_gotomap_make_empty(goto_container);
  /* for kernel item */
  KevKernelItem* kitem = itemset->items;
  for (; kitem; kitem = kitem->next) {
    KevRule* rule = kitem->rule;
    if (rule->bodylen == kitem->dot) continue;
    KevSymbol* symbol = rule->body[kitem->dot];
    KevKernelItem* item = kev_lr_kernel_item_create(rule, kitem->dot + 1);
    if (!item) return false;
    if (!(item->lookahead = kev_bitset_create_copy(kitem->lookahead))) {
      kev_lr_kernel_item_delete(item);
      return false;
    }
    KevGotoMapNode* node = kev_gotomap_search(goto_container, symbol);
    if (node) {
      kev_lr_itemset_add_item(node->value, item);
    } else {
      KevItemSet* iset = kev_lr_itemset_create();
      if (!iset) {
        kev_lr_kernel_item_delete(item);
        return false;
      }
      kev_lr_itemset_add_item(iset, item);
      if (!kev_lr_itemset_goto(itemset, symbol, iset) ||
          !kev_gotomap_insert(goto_container, symbol, iset)) {
        kev_lr_itemset_delete(iset);
        return false;
      }
    }
  }

  /* for non-kernel item */
  size_t closure_size = kev_addrarray_size(closure);
  for (size_t i = 0; i < closure_size; ++i) {
    KevSymbol* head = *(KevSymbol**)kev_addrarray_visit(closure, i);
    KevRuleNode* rulenode = head->rules;
    for (; rulenode; rulenode = rulenode->next) {
      KevRule* rule = rulenode->rule;
      if (rule->bodylen == 0) continue;
      KevSymbol* symbol = rule->body[0];
      KevKernelItem* item = kev_lr_kernel_item_create(rule, 1);
      if (!item) return false;
      if (!(item->lookahead = kev_bitset_create_copy(la_symbols[head->tmp_id]))) {
        kev_lr_kernel_item_delete(item);
        return false;
      }
      KevGotoMapNode* node = kev_gotomap_search(goto_container, symbol);
      if (node) {
        kev_lr_itemset_add_item(node->value, item);
      } else {
        KevItemSet* iset = kev_lr_itemset_create();
        if (!iset) {
          kev_lr_kernel_item_delete(item);
          return false;
        }
        kev_lr_itemset_add_item(iset, item);
        if (!kev_lr_itemset_goto(itemset, symbol, iset) ||
            !kev_gotomap_insert(goto_container, symbol, iset)) {
          kev_lr_itemset_delete(iset);
          return false;
        }
      }
    }
  }
  return true;
}

static inline bool kev_lalr_init_kitem_la(KevItemSet* itemset, size_t epsilon) {
  KevKernelItem* kitem = itemset->items;
  size_t id = epsilon + 1;
  while (kitem) {
      if (!kev_bitset_set(kitem->lookahead, id++)) {
      return false;
    }
    kitem = kitem->next;
  }
  return true;
}

static KevBitSet* kev_lalr_get_follows(KevKernelItem* kitem, KevBitSet** firsts, size_t epsilon) {
  size_t len = kitem->rule->bodylen;
  KevSymbol** rulebody = kitem->rule->body;
  KevBitSet* follows = kev_bitset_create(epsilon + 3);
  if (!follows) return NULL;
  for (size_t i = kitem->dot; i < len; ++i) {
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

static bool kev_lalr_closure(KevItemSet* itemset, KevAddrArray* closure, KevBitSet** la_symbols, KevBitSet** firsts, size_t epsilon) {
  if (!kev_lalr_init_kitem_la(itemset, epsilon)) return false;
  KevKernelItem* kitem = itemset->items;
  for (; kitem; kitem = kitem->next) {
    if (kitem->dot == kitem->rule->bodylen)
      continue;
    KevSymbol* symbol = kitem->rule->body[kitem->dot];
    if (symbol->kind == KEV_LR_SYMBOL_TERMINAL)
      continue;
    KevBitSet* la = kev_lalr_get_follows(kitem, firsts, epsilon);
    if (!la) return false;
    size_t index = symbol->tmp_id;
    if (la_symbols[index]) {
      if (!kev_bitset_union(la_symbols[index], la)) {
        kev_bitset_delete(la);
        return false;
      }
      kev_bitset_delete(la);
    } else {
      la_symbols[index] = la;
      if (!kev_addrarray_push_back(closure, symbol))
        return false;
    }
  }
  return kev_lalr_closure_propagate(itemset, closure, la_symbols, firsts, epsilon);
}

static bool kev_lalr_closure_propagate(KevItemSet* itemset, KevAddrArray* closure, KevBitSet** la_symbols, KevBitSet** firsts, size_t epsilon) {
  size_t closure_size = kev_addrarray_size(closure);
  bool propagated = true;
  while (propagated) {
    propagated = false;
    for (size_t i = 0; i < closure_size; ++i) {
      KevSymbol* symbol = *(KevSymbol**)kev_addrarray_visit(closure, i);
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

static bool kev_lalr_itemset_equal(KevItemSet* itemset1, KevItemSet* itemset2) {
  KevKernelItem* kitem1 = itemset1->items;
  KevKernelItem* kitem2 = itemset2->items;
  while (kitem1 && kitem2) {
    if (kitem1->rule != kitem2->rule || kitem1->dot != kitem2->dot)
      return false;
    kitem1 = kitem1->next;
    kitem2 = kitem2->next;
  }
  return !(kitem1 || kitem2);
}

static void kev_lalr_clear_la_symbols_and_closure(KevAddrArray* closure, KevBitSet** la_symbols) {
  size_t size = kev_addrarray_size(closure);
  for (size_t i = 0; i < size; ++i) {
    size_t index = ((KevSymbol*)kev_addrarray_visit(closure, i))->tmp_id;
    kev_bitset_delete(la_symbols[index]);
    la_symbols[index] = NULL;
  }
  kev_addrarray_make_empty(closure);
}

static bool kev_lalr_merge_itemset(KevItemSet* new, KevItemSet* old, KevItemSet* itemset, KevLALRCollection* collec) {
  KevLookaheadPropagation* propagation = collec->propagation;
  KevKernelItem* new_kitem = new->items;
  KevKernelItem* old_kitem = old->items;
  for (;old_kitem; old_kitem = old_kitem->next, new_kitem = new_kitem->next) {
    KevKernelItem* kitem_in_itemset = itemset->items;
    KevBitSet* old_la = old_kitem->lookahead;
    KevBitSet* new_la = new_kitem->lookahead;
    for (size_t i = collec->terminal_no + 1; kitem_in_itemset; kitem_in_itemset = kitem_in_itemset->next, ++i) {
      if (kev_bitset_has_element(new_la, i)) {
        kev_bitset_clear(new_la, i);
        KevLookaheadPropagation* new_propagation = kev_lalr_propagation_create(kitem_in_itemset->lookahead, old_la);
        if (!new_propagation) {
          collec->propagation = propagation;
          return false;
        }
        new_propagation->next = propagation;
        propagation = new_propagation;
      }
      if (!kev_bitset_union(old_la, new_la)) {
        collec->propagation = propagation;
        return false;
      }
    }
  }
  collec->propagation = propagation;
  return true;
}

static bool kev_lalr_add_new_itemset(KevItemSet* new, KevItemSetSet* iset_set, KevItemSet* itemset, KevLALRCollection* collec) {
  KevLookaheadPropagation* propagation = collec->propagation;
  for (KevKernelItem* kitem = new->items; kitem; kitem = kitem->next) {
    KevKernelItem* kitem_in_itemset = itemset->items;
    KevBitSet* la = kitem->lookahead;
    for (size_t i = collec->terminal_no + 1; kitem_in_itemset; kitem_in_itemset = kitem_in_itemset->next, ++i) {
      if (kev_bitset_has_element(la, i)) {
        kev_bitset_clear(la, i);
        KevLookaheadPropagation* new_propagation = kev_lalr_propagation_create(kitem_in_itemset->lookahead, la);
        if (!new_propagation) {
          collec->propagation = propagation;
          return false;
        }
        new_propagation->next = propagation;
        propagation = new_propagation;
      }
    }
  }
  collec->propagation = propagation;
  return kev_itemsetset_insert(iset_set, new);
}

static inline KevLookaheadPropagation* kev_lalr_propagation_create(KevBitSet* from, KevBitSet* to) {
  KevLookaheadPropagation* propagation = (KevLookaheadPropagation*)malloc(sizeof (KevLookaheadPropagation));
  if (!propagation) return NULL;
  propagation->from = from;
  propagation->to = to;
  return propagation;
}

static bool kev_lalr_do_lookahead_propagation(KevLookaheadPropagation* propagation) {
  bool propagation_done = false;
  while (!propagation_done) {
    propagation_done = true;
    for (KevLookaheadPropagation* prop = propagation; prop; prop = prop->next) {
      if (!kev_bitset_is_subset(prop->from, prop->to)) {
        if (!kev_bitset_union(prop->to, prop->from))
          return false;
        propagation_done = false;
      }
    }
  }
  return true;
}

static KevLRCollection* kev_lalr_to_lr_collec(KevLALRCollection* lalr_collec) {
  KevLRCollection* lr_collec = (KevLRCollection*)malloc(sizeof (KevLRCollection));
  if (!lr_collec) return NULL;
  lr_collec->firsts = lalr_collec->firsts;
  lr_collec->symbols = lalr_collec->symbols;
  lr_collec->terminal_no = lalr_collec->terminal_no;
  lr_collec->symbol_no = lalr_collec->symbol_no;
  lr_collec->itemsets = lalr_collec->itemsets;
  lr_collec->itemset_no = lalr_collec->itemset_no;
  lalr_collec->itemsets = NULL;
  lalr_collec->symbols = NULL;
  lalr_collec->firsts = NULL;
  kev_lalr_destroy_collec(lalr_collec);
  return lr_collec;
}
