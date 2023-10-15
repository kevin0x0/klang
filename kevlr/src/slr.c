#include "kevlr/include/collection.h"
#include "kevlr/include/lr_utils.h"
#include "kevlr/include/set/itemset_set.h"

#include <stdlib.h>

typedef struct tagKlrSLRCollection {
  KlrSymbol** symbols;
  size_t symbol_no;
  size_t terminal_no;
  KlrItemSet** itemsets;
  size_t itemset_no;
  KBitSet** firsts;
  KBitSet** follows;
  KlrSymbol* start;
  KlrRule* start_rule;
} KlrSLRCollection;

static bool klr_slr_get_all_itemsets(KlrItemSet* start_iset, KlrSLRCollection* collec);
static bool klr_slr_merge_transition(KlrItemSetSet* iset_set, KArray* itemset_array, KlrItemSet* itemset);
static bool klr_slr_get_itemset(KlrItemSet* itemset, KlrItemSetClosure* closure, KBitSet** firsts, KBitSet** follows, size_t epsilon, KlrTransMap* transitions);
/* initialize lookahead for kernel items in itemset */

static KlrSLRCollection* klr_slr_get_empty_collec(void);
static bool klr_slr_itemset_equal(KlrItemSet* itemset1, KlrItemSet* itemset2);
static KlrCollection* klr_slr_to_lr_collec(KlrSLRCollection* slr_collec);

static void klr_slr_destroy_itemset_array(KArray* itemset_array);
static void klr_slr_destroy_collec(KlrSLRCollection* collec);


KlrCollection* klr_collection_create_slr(KlrSymbol* start, KlrSymbol** ends, size_t ends_no) {
  KlrSLRCollection* collec = klr_slr_get_empty_collec();
  if (!collec) return NULL;
  KlrSymbol* augmented_grammar_start = klr_util_augment(start);
  if (!augmented_grammar_start) {
    klr_slr_destroy_collec(collec);
    return NULL;
  }
  collec->start = augmented_grammar_start;
  collec->start_rule = augmented_grammar_start->rules->rule;
  collec->symbols = klr_util_get_symbol_array(augmented_grammar_start, ends, ends_no, &collec->symbol_no);
  if (!collec->symbols) {
    klr_slr_destroy_collec(collec);
    return NULL;
  }
  collec->terminal_no = klr_util_symbol_array_partition(collec->symbols, collec->symbol_no);
  klr_util_label_symbols(collec->symbols, collec->symbol_no);
  collec->firsts = klr_util_compute_firsts(collec->symbols, collec->symbol_no, collec->terminal_no);
  if (!collec->firsts) {
    klr_slr_destroy_collec(collec);
    return NULL;
  }
  collec->follows = klr_util_compute_follows(collec->symbols, collec->firsts, collec->symbol_no, collec->terminal_no, collec->start, ends, ends_no);
  if (!collec->follows) {
    klr_slr_destroy_collec(collec);
    return NULL;
  }
  KlrItemSet* start_iset = klr_util_get_start_itemset(augmented_grammar_start, ends, ends_no);
  if (!start_iset) {
    klr_slr_destroy_collec(collec);
    return NULL;
  }
  if (!klr_slr_get_all_itemsets(start_iset, collec)) {
    klr_slr_destroy_collec(collec);
    return NULL;
  }
  klr_util_assign_itemset_id(collec->itemsets, collec->itemset_no);
  KlrCollection* lr_collec = klr_slr_to_lr_collec(collec);
  if (!lr_collec) klr_slr_destroy_collec(collec);
  return lr_collec;
}

static KlrSLRCollection* klr_slr_get_empty_collec(void) {
  KlrSLRCollection* collec = (KlrSLRCollection*)malloc(sizeof (KlrSLRCollection));
  if (!collec) return NULL;
  collec->firsts = NULL;
  collec->follows = NULL;
  collec->symbols = NULL;
  collec->itemsets = NULL;
  collec->start = NULL;
  collec->start_rule = NULL;
  return collec;
}

static void klr_slr_destroy_collec(KlrSLRCollection* collec) {
  if (collec->firsts)
    klr_util_destroy_terminal_set_array(collec->firsts, collec->symbol_no);
  if (collec->follows)
    klr_util_destroy_terminal_set_array(collec->follows, collec->symbol_no);
  if (collec->itemsets) {
    for (size_t i = 0; i < collec->itemset_no; ++i) {
      klr_itemset_delete(collec->itemsets[i]);
    }
    free(collec->itemsets);
  }
  if (collec->symbols)
    free(collec->symbols);
  klr_symbol_delete(collec->start);
  klr_rule_delete(collec->start_rule);
  free(collec);
}

static bool klr_slr_get_all_itemsets(KlrItemSet* start_iset, KlrSLRCollection* collec) {
  KArray* itemset_array = karray_create();
  KlrItemSetSet* iset_set = klr_itemsetset_create(16, klr_slr_itemset_equal);
  KlrItemSetClosure closure;
  KlrTransMap* goto_container = klr_transmap_create(16);
  size_t symbol_no = collec->symbol_no;
  if (!itemset_array || !goto_container || !iset_set || !klr_closure_init(&closure, symbol_no)) {
    karray_delete(itemset_array);
    klr_transmap_delete(goto_container);
    klr_itemsetset_delete(iset_set);
    klr_closure_destroy(&closure);
    return false;
  }
  
  if (!karray_push_back(itemset_array, start_iset) || !klr_itemsetset_insert(iset_set, start_iset)) {
    klr_slr_destroy_itemset_array(itemset_array);
    klr_transmap_delete(goto_container);
    klr_itemsetset_delete(iset_set);
    klr_closure_destroy(&closure);
    return false;
  }

  KBitSet** firsts = collec->firsts;
  size_t terminal_no = collec->terminal_no;
  for (size_t i = 0; i < karray_size(itemset_array); ++i) {
    KlrItemSet* itemset = (KlrItemSet*)karray_access(itemset_array, i);
    if (!klr_slr_get_itemset(itemset, &closure, firsts, collec->follows, terminal_no, goto_container)) {
      klr_slr_destroy_itemset_array(itemset_array);
      klr_transmap_delete(goto_container);
      klr_itemsetset_delete(iset_set);
      klr_closure_destroy(&closure);
      return false;
    }
    if (!klr_slr_merge_transition(iset_set, itemset_array, itemset)) {
      klr_slr_destroy_itemset_array(itemset_array);
      klr_transmap_delete(goto_container);
      klr_itemsetset_delete(iset_set);
      klr_closure_destroy(&closure);
      return false;
    }
    klr_closure_make_empty(&closure);
  }
  klr_transmap_delete(goto_container);
  klr_itemsetset_delete(iset_set);
  klr_closure_destroy(&closure);
  collec->itemset_no = karray_size(itemset_array);
  /* steal resources from itemset_array */
  collec->itemsets = (KlrItemSet**)karray_steal(itemset_array);
  karray_delete(itemset_array);
  return true;
}

static void klr_slr_destroy_itemset_array(KArray* itemset_array) {
  for (size_t i = 0; i < karray_size(itemset_array); ++i) {
    klr_itemset_delete((KlrItemSet*)karray_access(itemset_array, i));
  }
  karray_delete(itemset_array);
}

static bool klr_slr_merge_transition(KlrItemSetSet* iset_set, KArray* itemset_array, KlrItemSet* itemset) {
  KlrItemSetTransition* trans = itemset->trans;
  for (; trans; trans = trans->next) {
    KlrItemSet* target = trans->target;
    KlrItemSetSetNode* node = klr_itemsetset_search(iset_set, target);
    if (node) {
      klr_itemset_delete(target);
      trans->target = node->element;
    } else {
      if (!klr_itemsetset_insert(iset_set, target) ||
          !karray_push_back(itemset_array, target)) {
        for (; trans; trans = trans->next) {
          klr_itemset_delete(trans->target);
          trans->target = NULL;
        }
        return false;
      }
    }
  }
  return true;
}

bool klr_slr_generate_transition(KlrItemSet* itemset, KlrItemSetClosure* closure, KlrTransMap* transitions, KBitSet** follows) {
  klr_transmap_make_empty(transitions);
  KArray* symbols = closure->symbols;
  /* for kernel item */
  KlrItem* kitem = itemset->items;
  for (; kitem; kitem = kitem->next) {
    KlrRule* rule = kitem->rule;
    if (rule->bodylen == kitem->dot) continue;
    KlrSymbol* symbol = rule->body[kitem->dot];
    KlrItem* item = klr_item_create(rule, kitem->dot + 1);
    if (!item) return false;
    if (!(item->lookahead = kbitset_create_copy(follows[rule->head->index]))) {
      klr_item_delete(item);
      return false;
    }
    KlrTransMapNode* node = klr_transmap_search(transitions, symbol);
    if (node) {
      klr_itemset_add_item(node->value, item);
    } else {
      KlrItemSet* iset = klr_itemset_create();
      if (!iset) {
        klr_item_delete(item);
        return false;
      }
      klr_itemset_add_item(iset, item);
      if (!klr_itemset_goto(itemset, symbol, iset) ||
          !klr_transmap_insert(transitions, symbol, iset)) {
        klr_itemset_delete(iset);
        return false;
      }
    }
  }

  /* for non-kernel item */
  size_t closure_size = karray_size(symbols);
  for (size_t i = 0; i < closure_size; ++i) {
    KlrSymbol* head = (KlrSymbol*)karray_access(symbols, i);
    KlrRuleNode* rulenode = head->rules;
    for (; rulenode; rulenode = rulenode->next) {
      KlrRule* rule = rulenode->rule;
      if (rule->bodylen == 0) continue;
      KlrSymbol* symbol = rule->body[0];
      KlrItem* item = klr_item_create(rule, 1);
      if (!item) return false;
      if (!(item->lookahead = kbitset_create_copy(follows[rule->head->index]))) {
        klr_item_delete(item);
        return false;
      }
      KlrTransMapNode* node = klr_transmap_search(transitions, symbol);
      if (node) {
        klr_itemset_add_item(node->value, item);
      } else {
        KlrItemSet* iset = klr_itemset_create();
        if (!iset) {
          klr_item_delete(item);
          return false;
        }
        klr_itemset_add_item(iset, item);
        if (!klr_itemset_goto(itemset, symbol, iset) ||
            !klr_transmap_insert(transitions, symbol, iset)) {
          klr_itemset_delete(iset);
          return false;
        }
      }
    }
  }
  return true;
}

static bool klr_slr_get_itemset(KlrItemSet* itemset, KlrItemSetClosure* closure, KBitSet** firsts, KBitSet** follows, size_t epsilon, KlrTransMap* transitions) {
  if (!klr_closure_make(closure, itemset, firsts, epsilon))
    return false;
  if (!klr_slr_generate_transition(itemset, closure, transitions, follows))
    return false;
  return true;
}

static bool klr_slr_itemset_equal(KlrItemSet* itemset1, KlrItemSet* itemset2) {
  KlrItem* kitem1 = itemset1->items;
  KlrItem* kitem2 = itemset2->items;
  while (kitem1 && kitem2) {
    if (kitem1->rule != kitem2->rule || kitem1->dot != kitem2->dot) {
      return false;
    }
    kitem1 = kitem1->next;
    kitem2 = kitem2->next;
  }
  return !(kitem1 || kitem2);
}

static KlrCollection* klr_slr_to_lr_collec(KlrSLRCollection* slr_collec) {
  KlrCollection* lr_collec = (KlrCollection*)malloc(sizeof (KlrCollection));
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
  klr_slr_destroy_collec(slr_collec);
  return lr_collec;
}
