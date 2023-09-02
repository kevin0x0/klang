#include "pargen/include/lr/collection.h"
#include "pargen/include/lr/lr_utils.h"
#include "pargen/include/lr/set/itemset_set.h"

#include <stdlib.h>

typedef struct tagKevLR1Collection {
  KevSymbol** symbols;
  size_t symbol_no;
  size_t terminal_no;
  KevItemSet** itemsets;
  size_t itemset_no;
  KevBitSet** firsts;
  KevSymbol* start;
  KevRule* start_rule;
} KevLR1Collection;

static bool kev_lr1_get_all_itemsets(KevItemSet* start_iset, KevLR1Collection* collec);
static bool kev_lr1_merge_gotos(KevItemSetSet* iset_set, KevAddrArray* itemset_array, KevLR1Collection* collec, KevItemSet* itemset);
static bool kev_lr1_get_itemset(KevItemSet* itemset, KevItemSetClosure* closure, KevBitSet** firsts, size_t epsilon, KevGotoMap* goto_container);
/* initialize lookahead for kernel items in itemset */

static KevLR1Collection* kev_lr1_get_empty_collec(void);
static bool kev_lr1_itemset_equal(KevItemSet* itemset1, KevItemSet* itemset2);
static KevLRCollection* kev_lr1_to_lr_collec(KevLR1Collection* lalr_collec);

static void kev_lr1_destroy_itemset_array(KevAddrArray* itemset_array);
static void kev_lr1_destroy_collec(KevLR1Collection* collec);


KevLRCollection* kev_lr_collection_create_lr1(KevSymbol* start, KevSymbol** lookahead, size_t la_len) {
  KevLR1Collection* collec = kev_lr1_get_empty_collec();
  if (!collec) return NULL;
  KevSymbol* augmented_grammar_start = kev_lr_util_augment(start);
  if (!augmented_grammar_start) {
    kev_lr1_destroy_collec(collec);
    return NULL;
  }
  collec->start = augmented_grammar_start;
  collec->start_rule = augmented_grammar_start->rules->rule;
  collec->symbols = kev_lr_util_get_symbol_array(augmented_grammar_start, lookahead, la_len, &collec->symbol_no);
  if (!collec->symbols) {
    kev_lr1_destroy_collec(collec);
    return NULL;
  }
  kev_lr_util_symbol_array_partition(collec->symbols, collec->symbol_no);
  collec->terminal_no = kev_lr_util_label_symbols(collec->symbols, collec->symbol_no);
  collec->firsts = kev_lr_util_compute_first_array(collec->symbols, collec->symbol_no, collec->terminal_no);
  if (!collec->firsts) {
    kev_lr1_destroy_collec(collec);
    return NULL;
  }
  KevItemSet* start_iset = kev_lr_util_get_start_itemset(augmented_grammar_start, lookahead, la_len);
  if (!start_iset) {
    kev_lr1_destroy_collec(collec);
    return NULL;
  }
  if (!kev_lr1_get_all_itemsets(start_iset, collec)) {
    kev_lr1_destroy_collec(collec);
    return NULL;
  }
  kev_lr_util_assign_itemset_id(collec->itemsets, collec->itemset_no);
  KevLRCollection* lr_collec = kev_lr1_to_lr_collec(collec);
  if (!lr_collec) kev_lr1_destroy_collec(collec);
  return lr_collec;
}

static KevLR1Collection* kev_lr1_get_empty_collec(void) {
  KevLR1Collection* collec = (KevLR1Collection*)malloc(sizeof (KevLR1Collection));
  if (!collec) return NULL;
  collec->firsts = NULL;
  collec->symbols = NULL;
  collec->itemsets = NULL;
  collec->start = NULL;
  collec->start_rule = NULL;
  return collec;
}

static void kev_lr1_destroy_collec(KevLR1Collection* collec) {
  if (collec->firsts)
    kev_lr_util_destroy_first_array(collec->firsts, collec->symbol_no);
  if (collec->itemsets) {
    for (size_t i = 0; i < collec->itemset_no; ++i) {
      kev_lr_itemset_delete(collec->itemsets[i]);
    }
    free(collec->itemsets);
  }
  if (collec->symbols)
    free(collec->symbols);
  kev_lr_symbol_delete(collec->start);
  kev_lr_rule_delete(collec->start_rule);
  free(collec);
}

static bool kev_lr1_get_all_itemsets(KevItemSet* start_iset, KevLR1Collection* collec) {
  KevAddrArray* itemset_array = kev_addrarray_create();
  KevItemSetSet* iset_set = kev_itemsetset_create(16, kev_lr1_itemset_equal);
  KevItemSetClosure closure;
  KevGotoMap* goto_container = kev_gotomap_create(16);
  size_t symbol_no = collec->symbol_no;
  if (!itemset_array || !goto_container || !iset_set || !kev_lr_closure_init(&closure, symbol_no)) {
    kev_addrarray_delete(itemset_array);
    kev_gotomap_delete(goto_container);
    kev_itemsetset_delete(iset_set);
    kev_lr_closure_destroy(&closure);
    return false;
  }
  
  if (!kev_addrarray_push_back(itemset_array, start_iset) || !kev_itemsetset_insert(iset_set, start_iset)) {
    kev_lr1_destroy_itemset_array(itemset_array);
    kev_gotomap_delete(goto_container);
    kev_itemsetset_delete(iset_set);
    kev_lr_closure_destroy(&closure);
    return false;
  }

  KevBitSet** firsts = collec->firsts;
  size_t terminal_no = collec->terminal_no;
  for (size_t i = 0; i < kev_addrarray_size(itemset_array); ++i) {
    KevItemSet* itemset = kev_addrarray_visit(itemset_array, i);
    if (!kev_lr1_get_itemset(itemset, &closure, firsts, terminal_no, goto_container)) {
      kev_lr1_destroy_itemset_array(itemset_array);
      kev_gotomap_delete(goto_container);
      kev_itemsetset_delete(iset_set);
      kev_lr_closure_destroy(&closure);
      return false;
    }
    if (!kev_lr1_merge_gotos(iset_set, itemset_array, collec, itemset)) {
      kev_lr1_destroy_itemset_array(itemset_array);
      kev_gotomap_delete(goto_container);
      kev_itemsetset_delete(iset_set);
      kev_lr_closure_destroy(&closure);
      return false;
    }
    kev_lr_closure_make_empty(&closure);
  }
  kev_gotomap_delete(goto_container);
  kev_itemsetset_delete(iset_set);
  kev_lr_closure_destroy(&closure);
  /* steal resources from itemset_array */
  collec->itemsets = (KevItemSet**)itemset_array->begin;
  collec->itemset_no = kev_addrarray_size(itemset_array);
  itemset_array->begin = NULL;
  itemset_array->end = NULL;
  itemset_array->current = NULL;
  kev_addrarray_delete(itemset_array);
  return true;
}

static void kev_lr1_destroy_itemset_array(KevAddrArray* itemset_array) {
  for (size_t i = 0; i < kev_addrarray_size(itemset_array); ++i) {
    kev_lr_itemset_delete((KevItemSet*)kev_addrarray_visit(itemset_array, i));
  }
  kev_addrarray_delete(itemset_array);
}

static bool kev_lr1_merge_gotos(KevItemSetSet* iset_set, KevAddrArray* itemset_array, KevLR1Collection* collec, KevItemSet* itemset) {
  KevItemSetGoto* gotos = itemset->gotos;
  for (; gotos; gotos = gotos->next) {
    KevItemSet* target = gotos->itemset;
    KevItemSetSetNode* node = kev_itemsetset_search(iset_set, target);
    if (node) {
      kev_lr_itemset_delete(target);
      gotos->itemset = node->element;
    } else {
      if (!kev_itemsetset_insert(iset_set, target) ||
          !kev_addrarray_push_back(itemset_array, target)) {
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

static bool kev_lr1_get_itemset(KevItemSet* itemset, KevItemSetClosure* closure, KevBitSet** firsts, size_t epsilon, KevGotoMap* goto_container) {
  if (!kev_lr_closure_make(closure, itemset, firsts, epsilon))
    return false;
  if (!kev_lr_util_generate_gotos(itemset, closure, goto_container))
    return false;
  return true;
}

static bool kev_lr1_itemset_equal(KevItemSet* itemset1, KevItemSet* itemset2) {
  KevItem* kitem1 = itemset1->items;
  KevItem* kitem2 = itemset2->items;
  while (kitem1 && kitem2) {
    if (kitem1->rule != kitem2->rule || kitem1->dot != kitem2->dot ||
        !kev_bitset_equal(kitem1->lookahead, kitem2->lookahead)) {
      return false;
    }
    kitem1 = kitem1->next;
    kitem2 = kitem2->next;
  }
  return !(kitem1 || kitem2);
}

static KevLRCollection* kev_lr1_to_lr_collec(KevLR1Collection* lalr_collec) {
  KevLRCollection* lr_collec = (KevLRCollection*)malloc(sizeof (KevLRCollection));
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
  kev_lr1_destroy_collec(lalr_collec);
  return lr_collec;
}
