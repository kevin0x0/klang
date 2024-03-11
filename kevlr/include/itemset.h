#ifndef KEVCC_KEVLR_INCLUDE_ITEMSET_H
#define KEVCC_KEVLR_INCLUDE_ITEMSET_H

#include "kevlr/include/itemset_def.h"

#define klr_item_less_than(item1, item2) ((size_t)(item1) < (size_t)(item2) || \
                                            ((size_t)(item1) == (size_t)(item2) && \
                                            (item1)->dot < (item2)->dot))

static inline KlrItemSet* klr_itemset_create(KlrItemSetPool* pool);
void klr_itemset_delete(KlrItemPoolCollec* pool, KlrItemSet* itemset);
void klr_itemset_add_item(KlrItemSet* itemset, KlrItem* item);
static inline void klr_itemset_add_trans(KlrItemSet* itemset, KlrItemSetTransition* trans);
static inline bool klr_itemset_goto(KlrItemSetTransPool* pool, KlrItemSet* itemset, KlrSymbol* symbol, KlrItemSet* iset);

/* iterater for itemset */
static inline KlrItem* klr_itemset_iter_begin(KlrItemSet* itemset);
static inline KlrItem* klr_itemset_iter_next(KlrItem* item);
/* getter for itemset */
static inline KlrID klr_itemset_get_id(KlrItemSet* itemset);

static inline KlrItem* klr_item_create(KlrItemPool* pool, KlrRule* rule, size_t dot);
static inline KlrItem* klr_item_create_copy(KlrItemPool* pool, KlrItem* item);
static inline void klr_item_delete(KlrItemPool* pool, KlrItem* item);

/* item getter */
static inline KlrRule* klr_item_get_rule(KlrItem* item);
static inline KBitSet* klr_item_get_lookahead(KlrItem* item);
static inline size_t klr_item_get_dotpos(KlrItem* item);

bool klr_closure_init(KlrItemSetClosure* closure, size_t nsymbol);
void klr_closure_destroy(KlrItemSetClosure* closure);
KlrItemSetClosure* klr_closure_create(size_t nsymbol);
void klr_closure_delete(KlrItemSetClosure* closure);

bool klr_closure_make(KlrItemSetClosure* closure, KlrItemSet* itemset, KBitSet** firsts, size_t epsilon);
void klr_closure_make_empty(KlrItemSetClosure* closure);

static inline KlrItemSet* klr_itemset_create(KlrItemSetPool* pool) {
  KlrItemSet* itemset = klr_itemsetpool_allocate(pool);
  if (k_unlikely(!itemset)) return NULL;
  itemset->items = NULL;
  itemset->trans = NULL;
  return itemset;
}

static inline KlrItem* klr_item_create(KlrItemPool* pool, KlrRule* rule, size_t dot) {
  KlrItem* item = klr_itempool_allocate(pool);
  if (k_unlikely(!item)) return NULL;
  item->rule = rule;
  item->dot = dot;
  item->lookahead = NULL;
  return item;
}

static inline KlrItem* klr_item_create_copy(KlrItemPool* pool, KlrItem* item) {
  KlrItem* ret = klr_itempool_allocate(pool);
  ret->rule = item->rule;
  ret->dot = item->dot;
  ret->lookahead = NULL;
  if (k_unlikely(item->lookahead && !(ret->lookahead = kbitset_create_copy(item->lookahead)))) {
    klr_item_delete(pool, ret);
    return NULL;
  }
  return ret;
}

static inline void klr_item_delete(KlrItemPool* pool, KlrItem* item) {
  if (k_unlikely(!item)) return;
  kbitset_delete(item->lookahead);
  klr_itempool_deallocate(pool, item);
}

static inline void klr_itemset_add_trans(KlrItemSet* itemset, KlrItemSetTransition* trans) {
  trans->next = itemset->trans;
  itemset->trans = trans;
}

static inline bool klr_itemset_goto(KlrItemSetTransPool* pool, KlrItemSet* itemset, KlrSymbol* symbol, KlrItemSet* iset) {
  KlrItemSetTransition* trans = klr_itemsettranspool_allocate(pool);
  if (k_unlikely(!trans)) return false;
  trans->symbol = symbol;
  trans->target = iset;
  klr_itemset_add_trans(itemset, trans);
  return true;
}

static inline KlrRule* klr_item_get_rule(KlrItem* item) {
  return item->rule;
}

static inline KBitSet* klr_item_get_lookahead(KlrItem* item) {
  return item->lookahead;
}

static inline size_t klr_item_get_dotpos(KlrItem* item) {
  return item->dot;
}

static inline KlrItem* klr_itemset_iter_begin(KlrItemSet* itemset) {
  return itemset->items;
}

static inline KlrItem* klr_itemset_iter_next(KlrItem* item) {
  return item->next;
}

static inline size_t klr_itemset_get_id(KlrItemSet* itemset) {
  return itemset->id;
}

#endif
