#ifndef KEVCC_KEVLR_INCLUDE_ITEMSET_H
#define KEVCC_KEVLR_INCLUDE_ITEMSET_H

#include "kevlr/include/itemset_def.h"
#include "kevlr/include/object_pool/itemset_pool.h"
#include "kevlr/include/object_pool/itemsettrans_pool.h"
#include "kevlr/include/object_pool/item_pool.h"

#define klr_item_less_than(item1, item2) ((size_t)(item1) < (size_t)(item2) || \
                                            ((size_t)(item1) == (size_t)(item2) && \
                                            (item1)->dot < (item2)->dot))

static inline KlrItemSet* klr_itemset_create(void);
void klr_itemset_delete(KlrItemSet* itemset);
void klr_itemset_add_item(KlrItemSet* itemset, KlrItem* item);
static inline void klr_itemset_add_trans(KlrItemSet* itemset, KlrItemSetTransition* trans);
static inline bool klr_itemset_goto(KlrItemSet* itemset, KlrSymbol* symbol, KlrItemSet* iset);

/* iterate for itemset */
static inline KlrItem* klr_itemset_iter_begin(KlrItemSet* itemset);
static inline KlrItem* klr_itemset_iter_next(KlrItem* item);
/* get method for itemset */
static inline KlrID klr_itemset_get_id(KlrItemSet* itemset);

static inline KlrItem* klr_item_create(KlrRule* rule, size_t dot);
static inline KlrItem* klr_item_create_copy(KlrItem* item);
static inline void klr_item_delete(KlrItem* item);

/* item get method */
static inline KlrRule* klr_item_get_rule(KlrItem* item);
static inline KBitSet* klr_item_get_lookahead(KlrItem* item);
static inline size_t klr_item_get_dotpos(KlrItem* item);

bool klr_closure_init(KlrItemSetClosure* closure, size_t symbol_no);
void klr_closure_destroy(KlrItemSetClosure* closure);
KlrItemSetClosure* klr_closure_create(size_t symbol_no);
void klr_closure_delete(KlrItemSetClosure* closure);

bool klr_closure_make(KlrItemSetClosure* closure, KlrItemSet* itemset, KBitSet** firsts, size_t epsilon);
void klr_closure_make_empty(KlrItemSetClosure* closure);

static inline KlrItemSet* klr_itemset_create(void) {
  KlrItemSet* itemset = klr_itemset_pool_allocate();
  if (!itemset) return NULL;
  itemset->items = NULL;
  itemset->trans = NULL;
  return itemset;
}

static inline KlrItem* klr_item_create(KlrRule* rule, size_t dot) {
  KlrItem* item = klr_item_pool_allocate();
  if (!item) return NULL;
  item->rule = rule;
  item->dot = dot;
  item->lookahead = NULL;
  return item;
}

static inline KlrItem* klr_item_create_copy(KlrItem* item) {
  KlrItem* ret = klr_item_pool_allocate();
  ret->rule = item->rule;
  ret->dot = item->dot;
  ret->lookahead = NULL;
  if (item->lookahead && !(ret->lookahead = kbitset_create_copy(item->lookahead))) {
    klr_item_delete(ret);
    return NULL;
  }
  return ret;
}

static inline void klr_item_delete(KlrItem* item) {
  if (item) {
    kbitset_delete(item->lookahead);
    klr_item_pool_deallocate(item);
  }
}

static inline void klr_itemset_add_trans(KlrItemSet* itemset, KlrItemSetTransition* trans) {
  trans->next = itemset->trans;
  itemset->trans = trans;
}

static inline bool klr_itemset_goto(KlrItemSet* itemset, KlrSymbol* symbol, KlrItemSet* iset) {
  KlrItemSetTransition* trans = klr_itemsettrans_pool_allocate();
  if (!trans) return false;
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
