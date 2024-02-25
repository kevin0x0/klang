#include "kevlr/include/collection.h"
#include "kevlr/include/lr_utils.h"
#include "kevlr/include/set/itemset_set.h"

#include <stdlib.h>

typedef struct tagKlrLookaheadPropagation {
  KBitSet* from;
  KBitSet* to;
  struct tagKlrLookaheadPropagation* next;
} KlrLookaheadPropagation;

typedef struct tagKlrLALRCollection {
  KlrSymbol** symbols;
  size_t symbol_no;
  size_t terminal_no;
  KlrItemSet** itemsets;
  KlrLookaheadPropagation* propagation;
  size_t itemset_no;
  KBitSet** firsts;
  KlrSymbol* start;
  KlrRule* start_rule;
} KlrLALRCollection;


static bool klr_lalr_get_all_itemsets(KlrItemSet* start_iset, KlrLALRCollection* collec);
static bool klr_lalr_do_lookahead_propagation(KlrLookaheadPropagation* propagation);
static bool klr_lalr_merge_transition(KlrItemSetSet* iset_set, KArray* itemset_array, KlrLALRCollection* collec, KlrItemSet* itemset);
static bool klr_lalr_merge_itemset(KlrItemSet* new_itemset, KlrItemSet* old_itemset, KlrItemSet* itemset, KlrLALRCollection* collec);
static bool klr_lalr_add_new_itemset(KlrItemSet* new_itemset, KlrItemSetSet* iset_set, KlrItemSet* itemset, KlrLALRCollection* collec);
static bool klr_lalr_compute_transition(KlrItemSet* itemset, KlrItemSetClosure* closure, KBitSet** firsts, size_t epsilon, KlrTransSet* transitions);
/* initialize lookahead for kernel items in itemset */
static inline bool klr_lalr_init_kitem_la(KlrItemSet* itemset, size_t epsilon);
static inline void klr_lalr_final_kitem_la(KlrItemSet* itemset, size_t epsilon);

static inline void klr_lalr_assign_itemset_id(KlrItemSet** itemsets, size_t itemset_no);

static KlrLALRCollection* klr_lalr_get_empty_collec(void);
static bool klr_lalr_itemset_equal(KlrItemSet* itemset1, KlrItemSet* itemset2);
static inline KlrLookaheadPropagation* klr_lalr_propagation_create(KBitSet* from, KBitSet* to);
static KlrCollection* klr_lalr_to_lr_collec(KlrLALRCollection* lalr_collec);

static void klr_lalr_destroy_itemset_array(KArray* itemset_array);
static void klr_lalr_destroy_collec(KlrLALRCollection* collec);
static void klr_lalr_clear_trans(KlrItemSetTransition* trans);


KlrCollection* klr_collection_create_lalr(KlrSymbol* start, KlrSymbol** ends, size_t ends_no) {
  KlrLALRCollection* collec = klr_lalr_get_empty_collec();
  if (k_unlikely(!collec)) return NULL;
  KlrSymbol* augmented_grammar_start = klr_util_augment(start);
  if (!augmented_grammar_start) {
    klr_lalr_destroy_collec(collec);
    return NULL;
  }
  collec->start = augmented_grammar_start;
  collec->start_rule = augmented_grammar_start->rules->rule;
  collec->symbols = klr_util_get_symbol_array(augmented_grammar_start, ends, ends_no, &collec->symbol_no);
  if (k_unlikely(!collec->symbols)) {
    klr_lalr_destroy_collec(collec);
    return NULL;
  }
  collec->terminal_no = klr_util_symbol_array_partition(collec->symbols, collec->symbol_no);
  klr_util_label_symbols(collec->symbols, collec->symbol_no);
  collec->firsts = klr_util_compute_firsts(collec->symbols, collec->symbol_no, collec->terminal_no);
  if (k_unlikely(!collec->firsts)) {
    klr_lalr_destroy_collec(collec);
    return NULL;
  }
  KlrItemSet* start_iset = klr_util_get_start_itemset(augmented_grammar_start, ends, ends_no);
  if (k_unlikely(!start_iset)) {
    klr_lalr_destroy_collec(collec);
    return NULL;
  }
  if (k_unlikely(!klr_lalr_get_all_itemsets(start_iset, collec))) {
    klr_lalr_destroy_collec(collec);
    return NULL;
  }
  klr_lalr_assign_itemset_id(collec->itemsets, collec->itemset_no);
  if (k_unlikely(!klr_lalr_do_lookahead_propagation(collec->propagation))) {
    klr_lalr_destroy_collec(collec);
    return NULL;
  }
  KlrCollection* lr_collec = klr_lalr_to_lr_collec(collec);
  if (k_unlikely(!lr_collec)) klr_lalr_destroy_collec(collec);
  return lr_collec;
}

static KlrLALRCollection* klr_lalr_get_empty_collec(void) {
  KlrLALRCollection* collec = (KlrLALRCollection*)malloc(sizeof (KlrLALRCollection));
  if (k_unlikely(!collec)) return NULL;
  collec->firsts = NULL;
  collec->symbols = NULL;
  collec->itemsets = NULL;
  collec->propagation = NULL;
  collec->start = NULL;
  collec->start_rule = NULL;
  return collec;
}

static void klr_lalr_destroy_collec(KlrLALRCollection* collec) {
  if (collec->firsts)
    klr_util_destroy_terminal_set_array(collec->firsts, collec->symbol_no);
  if (collec->itemsets) {
    for (size_t i = 0; i < collec->itemset_no; ++i) {
      klr_itemset_delete(collec->itemsets[i]);
    }
    free(collec->itemsets);
  }
  if (collec->symbols)
    free(collec->symbols);
  if (collec->propagation) {
    KlrLookaheadPropagation* prop = collec->propagation;
    while (prop) {
      KlrLookaheadPropagation* tmp = prop->next;
      free(prop);
      prop = tmp;
    }
  }
  klr_symbol_delete(collec->start);
  klr_rule_delete(collec->start_rule);
  free(collec);
}

static bool klr_lalr_get_all_itemsets(KlrItemSet* start_iset, KlrLALRCollection* collec) {
  KArray* itemset_array = karray_create();
  KlrItemSetSet* iset_set = klr_itemsetset_create(32, klr_lalr_itemset_equal);
  /* to store the closure of a itemset */
  KlrItemSetClosure closure;
  size_t symbol_no = collec->symbol_no;
  KlrTransSet* transitions = klr_transset_create(symbol_no);
  if (k_unlikely(!itemset_array || !transitions || !iset_set || !klr_closure_init(&closure, symbol_no) ||
      !karray_push_back(itemset_array, start_iset) || !klr_itemsetset_insert(iset_set, start_iset))) {
    klr_lalr_destroy_itemset_array(itemset_array);
    klr_transset_delete(transitions);
    klr_itemsetset_delete(iset_set);
    klr_closure_destroy(&closure);
    return false;
  }
  
  KBitSet** firsts = collec->firsts;
  size_t terminal_no = collec->terminal_no;
  /* main loop */
  for (size_t i = 0; i < karray_size(itemset_array); ++i) {
    /* iterate all itemset int the itemset_array */
    KlrItemSet* itemset = (KlrItemSet*)karray_access(itemset_array, i);
    /* compute all transitions from this itemset
     * then add target itemsets of these transitions to itemset_array if not exist. */
    if (k_unlikely(!klr_lalr_compute_transition(itemset, &closure, firsts, terminal_no, transitions) ||
        !klr_lalr_merge_transition(iset_set, itemset_array, collec, itemset))) {
      klr_lalr_destroy_itemset_array(itemset_array);
      klr_transset_delete(transitions);
      klr_itemsetset_delete(iset_set);
      klr_closure_destroy(&closure);
      return false;
    }
    /* make it empty for use in next loop */
    klr_closure_make_empty(&closure);
  }

  /* clean */
  klr_transset_delete(transitions);
  klr_itemsetset_delete(iset_set);
  klr_closure_destroy(&closure);
  collec->itemset_no = karray_size(itemset_array);
  /* steal resources from itemset_array */
  collec->itemsets = (KlrItemSet**)karray_steal(itemset_array);
  karray_delete(itemset_array);
  return true;
}

static void klr_lalr_destroy_itemset_array(KArray* itemset_array) {
  if (k_unlikely(!itemset_array)) return;
  for (size_t i = 0; i < karray_size(itemset_array); ++i) {
    klr_itemset_delete((KlrItemSet*)karray_access(itemset_array, i));
  }
  karray_delete(itemset_array);
}

static void klr_lalr_clear_trans(KlrItemSetTransition* trans) {
  for (; trans; trans = trans->next) {
    klr_itemset_delete(trans->target);
    trans->target = NULL;
  }
}

static bool klr_lalr_merge_transition(KlrItemSetSet* iset_set, KArray* itemset_array, KlrLALRCollection* collec, KlrItemSet* itemset) {
  KlrItemSetTransition* trans = itemset->trans;
  /* iterate all transitions and add them to iset_set and itemset_array if not exists */
  for (; trans; trans = trans->next) {
    KlrItemSet* target = trans->target;
    /* search target in iset_set, which stores all computed itemsets */
    KlrItemSetSetNode* node = klr_itemsetset_search(iset_set, target);
    if (node) { /* is this target already exists ? */
      /* yes, merge them */
      if (k_unlikely(!klr_lalr_merge_itemset(target, node->element, itemset, collec))) {
        klr_lalr_clear_trans(trans);
        return false;
      }
      klr_itemset_delete(target);
      trans->target = node->element;
    } else {
      /* no, add this new itemset */
      if (k_unlikely(!klr_lalr_add_new_itemset(target, iset_set, itemset, collec) ||
          !karray_push_back(itemset_array, target))) {
        klr_lalr_clear_trans(trans);
        return false;
      }
    }
  }
  return true;
}

static bool klr_lalr_compute_transition(KlrItemSet* itemset, KlrItemSetClosure* closure, KBitSet** firsts, size_t epsilon, KlrTransSet* transitions) {
  /* 0 ... epsilon - 1 are indexes of terminal,
   * use epsilon + 1, epsilon + 2 ... as temporary propagation symbol for each kernel item */
  if (k_unlikely(!klr_lalr_init_kitem_la(itemset, epsilon)))
    return false;
  /* compute closure */
  if (k_unlikely(!klr_closure_make(closure, itemset, firsts, epsilon)))
    return false;
  /* compute all transitions starts from itemset */
  if (k_unlikely(!klr_util_generate_transition(itemset, closure, transitions)))
    return false;
  /* clean the temporary propagation symbol */
  klr_lalr_final_kitem_la(itemset, epsilon);
  return true;
}

static inline bool klr_lalr_init_kitem_la(KlrItemSet* itemset, size_t epsilon) {
  KlrItem* kitem = itemset->items;
  size_t id = epsilon + 1;
  for (; kitem; kitem = kitem->next) {
    if (!kbitset_set(kitem->lookahead, id++))
      return false;
  }
  return true;
}

static inline void klr_lalr_final_kitem_la(KlrItemSet* itemset, size_t epsilon) {
  KlrItem* kitem = itemset->items;
  size_t id = epsilon + 1;
  for (; kitem; kitem = kitem->next) {
    kbitset_clear(kitem->lookahead, id++);
  }
}


static bool klr_lalr_itemset_equal(KlrItemSet* itemset1, KlrItemSet* itemset2) {
  KlrItem* kitem1 = itemset1->items;
  KlrItem* kitem2 = itemset2->items;
  while (kitem1 && kitem2) {
    if (kitem1->rule != kitem2->rule || kitem1->dot != kitem2->dot)
      return false;
    kitem1 = kitem1->next;
    kitem2 = kitem2->next;
  }
  return !(kitem1 || kitem2);
}

static bool klr_lalr_merge_itemset(KlrItemSet* new_itemset, KlrItemSet* old_itemset, KlrItemSet* itemset, KlrLALRCollection* collec) {
  KlrLookaheadPropagation* propagation = collec->propagation;
  KlrItem* new_kitem = new_itemset->items;
  KlrItem* old_kitem = old_itemset->items;
  size_t epsilon = collec->terminal_no;
  for (;old_kitem; old_kitem = old_kitem->next, new_kitem = new_kitem->next) {
    KlrItem* kitem_in_itemset = itemset->items;
    KBitSet* old_la = old_kitem->lookahead;
    KBitSet* new_la = new_kitem->lookahead;
    for (size_t i = epsilon + 1; kitem_in_itemset; kitem_in_itemset = kitem_in_itemset->next, ++i) {
      if (!kbitset_has_element(new_la, i))
        continue;
      /* lookaheads of new itemset include i,
       * signals this kitem in itemset propagate symbols to this kernel corresponding target  */

      /* clear the temporary propagation symbol */
      kbitset_clear(new_la, i);
      /* remember the propagation */
      KlrLookaheadPropagation* new_propagation = klr_lalr_propagation_create(kitem_in_itemset->lookahead, old_la);
      if (k_unlikely(!new_propagation)) {
        collec->propagation = propagation;
        return false;
      }
      new_propagation->next = propagation;
      propagation = new_propagation;
    }
    /* merge the new one to old one */
    if (k_unlikely(!kbitset_union(old_la, new_la)))
      return false;
  }
  collec->propagation = propagation;
  return true;
}

static bool klr_lalr_add_new_itemset(KlrItemSet* new_itemset, KlrItemSetSet* iset_set, KlrItemSet* itemset, KlrLALRCollection* collec) {
  KlrLookaheadPropagation* propagation = collec->propagation;
  for (KlrItem* kitem = new_itemset->items; kitem; kitem = kitem->next) {
    KlrItem* kitem_in_itemset = itemset->items;
    KBitSet* la = kitem->lookahead;
    for (size_t i = collec->terminal_no + 1; kitem_in_itemset; kitem_in_itemset = kitem_in_itemset->next, ++i) {
      if (!kbitset_has_element(la, i))
        continue;
      /* like klr_lalr_merge_itemset() */
      kbitset_clear(la, i);
      KlrLookaheadPropagation* new_propagation = klr_lalr_propagation_create(kitem_in_itemset->lookahead, la);
      if (!new_propagation) {
        collec->propagation = propagation;
        return false;
      }
      new_propagation->next = propagation;
      propagation = new_propagation;
    }
  }
  collec->propagation = propagation;
  /* this is new itemset, so add it to iset_set */
  return klr_itemsetset_insert(iset_set, new_itemset);
}

static inline KlrLookaheadPropagation* klr_lalr_propagation_create(KBitSet* from, KBitSet* to) {
  KlrLookaheadPropagation* propagation = (KlrLookaheadPropagation*)malloc(sizeof (KlrLookaheadPropagation));
  if (k_unlikely(!propagation)) return NULL;
  propagation->from = from;
  propagation->to = to;
  return propagation;
}

static bool klr_lalr_do_lookahead_propagation(KlrLookaheadPropagation* propagation) {
  bool propagation_done = false;
  while (!propagation_done) {
    propagation_done = true;
    for (KlrLookaheadPropagation* prop = propagation; prop; prop = prop->next) {
      if (!kbitset_is_subset(prop->from, prop->to)) {
        if (k_unlikely(!kbitset_union(prop->to, prop->from)))
          return false;
        propagation_done = false;
      }
    }
  }
  return true;
}

static KlrCollection* klr_lalr_to_lr_collec(KlrLALRCollection* lalr_collec) {
  KlrCollection* lr_collec = (KlrCollection*)malloc(sizeof (KlrCollection));
  if (!lr_collec) return NULL;
  lr_collec->firsts = lalr_collec->firsts;
  lr_collec->symbols = lalr_collec->symbols;
  lr_collec->terminal_no = lalr_collec->terminal_no;
  lr_collec->symbol_no = lalr_collec->symbol_no;
  lr_collec->itemsets = lalr_collec->itemsets;
  lr_collec->itemset_no = lalr_collec->itemset_no;
  lr_collec->start = lalr_collec->start;
  lr_collec->start_rule = lalr_collec->start_rule;
  lalr_collec->itemsets = NULL;
  lalr_collec->symbols = NULL;
  lalr_collec->firsts = NULL;
  lalr_collec->start = NULL;
  lalr_collec->start_rule = NULL;
  klr_lalr_destroy_collec(lalr_collec);
  return lr_collec;
}

static inline void klr_lalr_assign_itemset_id(KlrItemSet** itemsets, size_t itemset_no) {
  for (size_t i = 0; i < itemset_no; ++i)
    itemsets[i]->id = i;
}
