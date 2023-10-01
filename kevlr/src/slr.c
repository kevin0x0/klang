#include "kevlr/include/collection.h"
#include "kevlr/include/lr_utils.h"
#include "kevlr/include/set/itemset_set.h"

#include <stdlib.h>

typedef struct tagKevSLRCollection {
  KevSymbol** symbols;
  size_t symbol_no;
  size_t terminal_no;
  KevItemSet** itemsets;
  size_t itemset_no;
  KevBitSet** firsts;
  KevBitSet** follows;
  KevSymbol* start;
  KevRule* start_rule;
} KevSLRCollection;

static bool kev_slr_get_all_itemsets(KevItemSet* start_iset, KevSLRCollection* collec);
static bool kev_slr_merge_gotos(KevItemSetSet* iset_set, KevAddrArray* itemset_array, KevItemSet* itemset);
static bool kev_slr_get_itemset(KevItemSet* itemset, KevItemSetClosure* closure, KevBitSet** firsts, KevBitSet** follows, size_t epsilon, KevGotoMap* goto_container);
/* initialize lookahead for kernel items in itemset */

static KevSLRCollection* kev_slr_get_empty_collec(void);
static bool kev_slr_itemset_equal(KevItemSet* itemset1, KevItemSet* itemset2);
static KevLRCollection* kev_slr_to_lr_collec(KevSLRCollection* slr_collec);

static void kev_slr_destroy_itemset_array(KevAddrArray* itemset_array);
static void kev_slr_destroy_collec(KevSLRCollection* collec);


KevLRCollection* kev_lr_collection_create_slr(KevSymbol* start, KevSymbol** ends, size_t ends_no) {
  KevSLRCollection* collec = kev_slr_get_empty_collec();
  if (!collec) return NULL;
  KevSymbol* augmented_grammar_start = kev_lr_util_augment(start);
  if (!augmented_grammar_start) {
    kev_slr_destroy_collec(collec);
    return NULL;
  }
  collec->start = augmented_grammar_start;
  collec->start_rule = augmented_grammar_start->rules->rule;
  collec->symbols = kev_lr_util_get_symbol_array(augmented_grammar_start, ends, ends_no, &collec->symbol_no);
  if (!collec->symbols) {
    kev_slr_destroy_collec(collec);
    return NULL;
  }
  kev_lr_util_symbol_array_partition(collec->symbols, collec->symbol_no);
  collec->terminal_no = kev_lr_util_label_symbols(collec->symbols, collec->symbol_no);
  collec->firsts = kev_lr_util_compute_firsts(collec->symbols, collec->symbol_no, collec->terminal_no);
  if (!collec->firsts) {
    kev_slr_destroy_collec(collec);
    return NULL;
  }
  collec->follows = kev_lr_util_compute_follows(collec->symbols, collec->firsts, collec->symbol_no, collec->terminal_no, collec->start, ends, ends_no);
  if (!collec->follows) {
    kev_slr_destroy_collec(collec);
    return NULL;
  }
  KevItemSet* start_iset = kev_lr_util_get_start_itemset(augmented_grammar_start, ends, ends_no);
  if (!start_iset) {
    kev_slr_destroy_collec(collec);
    return NULL;
  }
  if (!kev_slr_get_all_itemsets(start_iset, collec)) {
    kev_slr_destroy_collec(collec);
    return NULL;
  }
  kev_lr_util_assign_itemset_id(collec->itemsets, collec->itemset_no);
  KevLRCollection* lr_collec = kev_slr_to_lr_collec(collec);
  if (!lr_collec) kev_slr_destroy_collec(collec);
  return lr_collec;
}

static KevSLRCollection* kev_slr_get_empty_collec(void) {
  KevSLRCollection* collec = (KevSLRCollection*)malloc(sizeof (KevSLRCollection));
  if (!collec) return NULL;
  collec->firsts = NULL;
  collec->follows = NULL;
  collec->symbols = NULL;
  collec->itemsets = NULL;
  collec->start = NULL;
  collec->start_rule = NULL;
  return collec;
}

static void kev_slr_destroy_collec(KevSLRCollection* collec) {
  if (collec->firsts)
    kev_lr_util_destroy_terminal_set_array(collec->firsts, collec->symbol_no);
  if (collec->follows)
    kev_lr_util_destroy_terminal_set_array(collec->follows, collec->symbol_no);
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

static bool kev_slr_get_all_itemsets(KevItemSet* start_iset, KevSLRCollection* collec) {
  KevAddrArray* itemset_array = kev_addrarray_create();
  KevItemSetSet* iset_set = kev_itemsetset_create(16, kev_slr_itemset_equal);
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
    kev_slr_destroy_itemset_array(itemset_array);
    kev_gotomap_delete(goto_container);
    kev_itemsetset_delete(iset_set);
    kev_lr_closure_destroy(&closure);
    return false;
  }

  KevBitSet** firsts = collec->firsts;
  size_t terminal_no = collec->terminal_no;
  for (size_t i = 0; i < kev_addrarray_size(itemset_array); ++i) {
    KevItemSet* itemset = (KevItemSet*)kev_addrarray_visit(itemset_array, i);
    if (!kev_slr_get_itemset(itemset, &closure, firsts, collec->follows, terminal_no, goto_container)) {
      kev_slr_destroy_itemset_array(itemset_array);
      kev_gotomap_delete(goto_container);
      kev_itemsetset_delete(iset_set);
      kev_lr_closure_destroy(&closure);
      return false;
    }
    if (!kev_slr_merge_gotos(iset_set, itemset_array, itemset)) {
      kev_slr_destroy_itemset_array(itemset_array);
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
  collec->itemset_no = kev_addrarray_size(itemset_array);
  /* steal resources from itemset_array */
  collec->itemsets = (KevItemSet**)kev_addrarray_steal(itemset_array);
  kev_addrarray_delete(itemset_array);
  return true;
}

static void kev_slr_destroy_itemset_array(KevAddrArray* itemset_array) {
  for (size_t i = 0; i < kev_addrarray_size(itemset_array); ++i) {
    kev_lr_itemset_delete((KevItemSet*)kev_addrarray_visit(itemset_array, i));
  }
  kev_addrarray_delete(itemset_array);
}

static bool kev_slr_merge_gotos(KevItemSetSet* iset_set, KevAddrArray* itemset_array, KevItemSet* itemset) {
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

bool kev_slr_generate_gotos(KevItemSet* itemset, KevItemSetClosure* closure, KevGotoMap* goto_container, KevBitSet** follows) {
  kev_gotomap_make_empty(goto_container);
  KevAddrArray* symbols = closure->symbols;
  /* for kernel item */
  KevItem* kitem = itemset->items;
  for (; kitem; kitem = kitem->next) {
    KevRule* rule = kitem->rule;
    if (rule->bodylen == kitem->dot) continue;
    KevSymbol* symbol = rule->body[kitem->dot];
    KevItem* item = kev_lr_item_create(rule, kitem->dot + 1);
    if (!item) return false;
    if (!(item->lookahead = kev_bitset_create_copy(follows[rule->head->index]))) {
      kev_lr_item_delete(item);
      return false;
    }
    KevGotoMapNode* node = kev_gotomap_search(goto_container, symbol);
    if (node) {
      kev_lr_itemset_add_item(node->value, item);
    } else {
      KevItemSet* iset = kev_lr_itemset_create();
      if (!iset) {
        kev_lr_item_delete(item);
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
  size_t closure_size = kev_addrarray_size(symbols);
  for (size_t i = 0; i < closure_size; ++i) {
    KevSymbol* head = (KevSymbol*)kev_addrarray_visit(symbols, i);
    KevRuleNode* rulenode = head->rules;
    for (; rulenode; rulenode = rulenode->next) {
      KevRule* rule = rulenode->rule;
      if (rule->bodylen == 0) continue;
      KevSymbol* symbol = rule->body[0];
      KevItem* item = kev_lr_item_create(rule, 1);
      if (!item) return false;
      if (!(item->lookahead = kev_bitset_create_copy(follows[rule->head->index]))) {
        kev_lr_item_delete(item);
        return false;
      }
      KevGotoMapNode* node = kev_gotomap_search(goto_container, symbol);
      if (node) {
        kev_lr_itemset_add_item(node->value, item);
      } else {
        KevItemSet* iset = kev_lr_itemset_create();
        if (!iset) {
          kev_lr_item_delete(item);
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

static bool kev_slr_get_itemset(KevItemSet* itemset, KevItemSetClosure* closure, KevBitSet** firsts, KevBitSet** follows, size_t epsilon, KevGotoMap* goto_container) {
  if (!kev_lr_closure_make(closure, itemset, firsts, epsilon))
    return false;
  if (!kev_slr_generate_gotos(itemset, closure, goto_container, follows))
    return false;
  return true;
}

static bool kev_slr_itemset_equal(KevItemSet* itemset1, KevItemSet* itemset2) {
  KevItem* kitem1 = itemset1->items;
  KevItem* kitem2 = itemset2->items;
  while (kitem1 && kitem2) {
    if (kitem1->rule != kitem2->rule || kitem1->dot != kitem2->dot) {
      return false;
    }
    kitem1 = kitem1->next;
    kitem2 = kitem2->next;
  }
  return !(kitem1 || kitem2);
}

static KevLRCollection* kev_slr_to_lr_collec(KevSLRCollection* slr_collec) {
  KevLRCollection* lr_collec = (KevLRCollection*)malloc(sizeof (KevLRCollection));
  if (!lr_collec) return NULL;
  lr_collec->firsts = slr_collec->firsts;
  lr_collec->symbols = slr_collec->symbols;
  lr_collec->terminal_no = slr_collec->terminal_no;
  lr_collec->symbol_no = slr_collec->symbol_no;
  lr_collec->itemsets = slr_collec->itemsets;
  lr_collec->itemset_no = slr_collec->itemset_no;
  lr_collec->start = slr_collec->start;
  lr_collec->start_rule = slr_collec->start_rule;
  slr_collec->itemsets = NULL;
  slr_collec->symbols = NULL;
  slr_collec->firsts = NULL;
  slr_collec->start = NULL;
  slr_collec->start_rule = NULL;
  kev_slr_destroy_collec(slr_collec);
  return lr_collec;
}
