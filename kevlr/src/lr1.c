#include "kevlr/include/collection.h"
#include "kevlr/include/itemset_def.h"
#include "kevlr/include/lr_utils.h"
#include "kevlr/include/set/itemset_set.h"

#include <stdlib.h>

typedef struct tagKlrLR1Collection {
  KlrSymbol** symbols;
  size_t symbol_no;
  size_t terminal_no;
  KlrItemSet** itemsets;
  size_t itemset_no;
  KBitSet** firsts;
  KlrSymbol* start;
  KlrRule* start_rule;
  KlrItemPoolCollec pool;
} KlrLR1Collection;

static bool klr_lr1_get_all_itemsets(KlrItemSet* start_iset, KlrLR1Collection* collec);
static bool klr_lr1_merge_transition(KlrItemPoolCollec* pool, KlrItemSetSet* iset_set, KArray* itemset_array, KlrItemSet* itemset);
static bool klr_lr1_compute_transition(KlrItemPoolCollec* pool, KlrItemSet* itemset, KlrItemSetClosure* closure, KBitSet** firsts, size_t epsilon, KlrTransSet* transitions);
/* initialize lookahead for kernel items in itemset */

static KlrLR1Collection* klr_lr1_get_empty_collec(void);
static bool klr_lr1_itemset_equal(KlrItemSet* itemset1, KlrItemSet* itemset2);
static KlrCollection* klr_lr1_to_lr_collec(KlrLR1Collection* slr_collec);

static void klr_lr1_destroy_itemset_array(KlrItemPoolCollec* pool, KArray* itemset_array);
static void klr_lr1_destroy_collec(KlrLR1Collection* collec);


KlrCollection* klr_collection_create_lr1(KlrSymbol* start, KlrSymbol** ends, size_t nend) {
  KlrLR1Collection* collec = klr_lr1_get_empty_collec();
  if (k_unlikely(!collec)) return NULL;
  KlrSymbol* augmented_grammar_start = klr_util_augment(start);
  if (k_unlikely(!augmented_grammar_start)) {
    klr_lr1_destroy_collec(collec);
    return NULL;
  }
  collec->start = augmented_grammar_start;
  collec->start_rule = augmented_grammar_start->rules->rule;
  collec->symbols = klr_util_get_symbol_array(augmented_grammar_start, ends, nend, &collec->symbol_no);
  if (k_unlikely(!collec->symbols)) {
    klr_lr1_destroy_collec(collec);
    return NULL;
  }
  collec->terminal_no = klr_util_symbol_array_partition(collec->symbols, collec->symbol_no);
  klr_util_label_symbols(collec->symbols, collec->symbol_no);
  collec->firsts = klr_util_compute_firsts(collec->symbols, collec->symbol_no, collec->terminal_no);
  if (k_unlikely(!collec->firsts)) {
    klr_lr1_destroy_collec(collec);
    return NULL;
  }
  KlrItemSet* start_iset = klr_util_get_start_itemset(&collec->pool, augmented_grammar_start, ends, nend);
  if (k_unlikely(!start_iset)) {
    klr_lr1_destroy_collec(collec);
    return NULL;
  }
  if (k_unlikely(!klr_lr1_get_all_itemsets(start_iset, collec))) {
    klr_lr1_destroy_collec(collec);
    return NULL;
  }
  klr_util_label_itemsets(collec->itemsets, collec->itemset_no);
  KlrCollection* lr_collec = klr_lr1_to_lr_collec(collec);
  if (k_unlikely(!lr_collec)) klr_lr1_destroy_collec(collec);
  return lr_collec;
}

static KlrLR1Collection* klr_lr1_get_empty_collec(void) {
  KlrLR1Collection* collec = (KlrLR1Collection*)malloc(sizeof (KlrLR1Collection));
  if (k_unlikely(!collec)) return NULL;
  collec->firsts = NULL;
  collec->symbols = NULL;
  collec->itemsets = NULL;
  collec->start = NULL;
  collec->start_rule = NULL;
  klr_itempoolcollec_init(&collec->pool);
  return collec;
}

static void klr_lr1_destroy_collec(KlrLR1Collection* collec) {
  if (collec->firsts)
    klr_util_destroy_terminal_set_array(collec->firsts, collec->symbol_no);
  if (collec->itemsets) {
    for (size_t i = 0; i < collec->itemset_no; ++i) {
      klr_itemset_delete(&collec->pool, collec->itemsets[i]);
    }
    free(collec->itemsets);
  }
  if (collec->symbols)
    free(collec->symbols);
  klr_symbol_delete(collec->start);
  klr_rule_delete(collec->start_rule);
  klr_itempoolcollec_destroy(&collec->pool);
  free(collec);
}

static bool klr_lr1_get_all_itemsets(KlrItemSet* start_iset, KlrLR1Collection* collec) {
  KArray* itemset_array = karray_create();
  KlrItemSetSet* iset_set = klr_itemsetset_create(32, klr_lr1_itemset_equal);
  KlrItemSetClosure closure;
  size_t nsymbol = collec->symbol_no;
  KlrTransSet* transitions = klr_transset_create(nsymbol);
  if (k_unlikely(!itemset_array || !transitions || !iset_set || !klr_closure_init(&closure, nsymbol))) {
    karray_delete(itemset_array);
    klr_transset_delete(transitions);
    klr_itemsetset_delete(iset_set);
    klr_closure_destroy(&closure);
    return false;
  }
  
  if (k_unlikely(!karray_push_back(itemset_array, start_iset) || !klr_itemsetset_insert(iset_set, start_iset))) {
    klr_lr1_destroy_itemset_array(&collec->pool, itemset_array);
    klr_transset_delete(transitions);
    klr_itemsetset_delete(iset_set);
    klr_closure_destroy(&closure);
    return false;
  }

  KBitSet** firsts = collec->firsts;
  size_t nterminal = collec->terminal_no;
  /* main loop */
  for (size_t i = 0; i < karray_size(itemset_array); ++i) {
    KlrItemSet* itemset = (KlrItemSet*)karray_access(itemset_array, i);
    if (k_unlikely(!klr_lr1_compute_transition(&collec->pool, itemset, &closure, firsts, nterminal, transitions))) {
      klr_lr1_destroy_itemset_array(&collec->pool, itemset_array);
      klr_transset_delete(transitions);
      klr_itemsetset_delete(iset_set);
      klr_closure_destroy(&closure);
      return false;
    }
    if (k_unlikely(!klr_lr1_merge_transition(&collec->pool, iset_set, itemset_array, itemset))) {
      klr_lr1_destroy_itemset_array(&collec->pool, itemset_array);
      klr_transset_delete(transitions);
      klr_itemsetset_delete(iset_set);
      klr_closure_destroy(&closure);
      return false;
    }
    klr_closure_make_empty(&closure);
  }

  klr_transset_delete(transitions);
  klr_itemsetset_delete(iset_set);
  klr_closure_destroy(&closure);
  collec->itemset_no = karray_size(itemset_array);
  /* steal resources from itemset_array */
  collec->itemsets = (KlrItemSet**)karray_steal(itemset_array);
  karray_delete(itemset_array);
  return true;
}

static void klr_lr1_destroy_itemset_array(KlrItemPoolCollec* pool, KArray* itemset_array) {
  for (size_t i = 0; i < karray_size(itemset_array); ++i) {
    klr_itemset_delete(pool, (KlrItemSet*)karray_access(itemset_array, i));
  }
  karray_delete(itemset_array);
}

static bool klr_lr1_merge_transition(KlrItemPoolCollec* pool, KlrItemSetSet* iset_set, KArray* itemset_array, KlrItemSet* itemset) {
  KlrItemSetTransition* trans = itemset->trans;
  for (; trans; trans = trans->next) {
    KlrItemSet* target = trans->target;
    KlrItemSetSetNode* node = klr_itemsetset_search(iset_set, target);
    if (node) {
      klr_itemset_delete(pool, target);
      trans->target = node->element;
    } else {
      if (k_unlikely(!klr_itemsetset_insert(iset_set, target) ||
          !karray_push_back(itemset_array, target))) {
        for (; trans; trans = trans->next) {
          klr_itemset_delete(pool, trans->target);
          trans->target = NULL;
        }
        return false;
      }
    }
  }
  return true;
}

static bool klr_lr1_compute_transition(KlrItemPoolCollec* pool, KlrItemSet* itemset, KlrItemSetClosure* closure, KBitSet** firsts, size_t epsilon, KlrTransSet* transitions) {
  if (k_unlikely(!klr_closure_make(closure, itemset, firsts, epsilon)))
    return false;
  if (k_unlikely(!klr_util_generate_transition(pool, itemset, closure, transitions)))
    return false;
  return true;
}

static bool klr_lr1_itemset_equal(KlrItemSet* itemset1, KlrItemSet* itemset2) {
  KlrItem* kitem1 = itemset1->items;
  KlrItem* kitem2 = itemset2->items;
  while (kitem1 && kitem2) {
    if (kitem1->rule != kitem2->rule || kitem1->dot != kitem2->dot ||
        !kbitset_equal(kitem1->lookahead, kitem2->lookahead)) {
      return false;
    }
    kitem1 = kitem1->next;
    kitem2 = kitem2->next;
  }
  return !(kitem1 || kitem2);
}

static KlrCollection* klr_lr1_to_lr_collec(KlrLR1Collection* lr1_collec) {
  KlrCollection* lr_collec = (KlrCollection*)malloc(sizeof (KlrCollection));
  if (!lr_collec) return NULL;
  lr_collec->firsts = lr1_collec->firsts;
  lr_collec->symbols = lr1_collec->symbols;
  lr_collec->nterminal = lr1_collec->terminal_no;
  lr_collec->nsymbol = lr1_collec->symbol_no;
  lr_collec->itemsets = lr1_collec->itemsets;
  lr_collec->nitemset = lr1_collec->itemset_no;
  lr_collec->start = lr1_collec->start;
  lr_collec->start_rule = lr1_collec->start_rule;
  lr_collec->pool = lr1_collec->pool;
  lr1_collec->itemsets = NULL;
  lr1_collec->symbols = NULL;
  lr1_collec->firsts = NULL;
  lr1_collec->start = NULL;
  lr1_collec->start_rule = NULL;
  klr_itempoolcollec_init(&lr1_collec->pool);
  klr_lr1_destroy_collec(lr1_collec);
  return lr_collec;
}
