#ifndef KEVCC_PARGEN_INCLUDE_LR_ITEM_H
#define KEVCC_PARGEN_INCLUDE_LR_ITEM_H

#include "pargen/include/lr/item_def.h"
#include "pargen/include/lr/object_pool/itemset_pool.h"
#include "pargen/include/lr/object_pool/itemsetgoto_pool.h"
#include "pargen/include/lr/object_pool/kernel_item_pool.h"

#include <stdlib.h>

#define kev_lr_item_less_than(item1, item2) ((size_t)(item1) < (size_t)(item2) || \
                                            ((size_t)(item1) == (size_t)(item2) && \
                                            (item1)->dot < (item2)->dot))

static inline KevItemSet* kev_lr_itemset_create(void);
void kev_lr_itemset_delete(KevItemSet* itemset);
void kev_lr_itemset_add_item(KevItemSet* itemset, KevItem* item);
static inline void kev_lr_itemset_add_goto(KevItemSet* itemset, KevItemSetGoto* go_to);
static inline bool kev_lr_itemset_goto(KevItemSet* itemset, KevSymbol* symbol, KevItemSet* iset);

static inline KevItem* kev_lr_item_create(KevRule* rule, size_t dot);
static inline KevItem* kev_lr_item_create_copy(KevItem* item);
static inline void kev_lr_item_delete(KevItem* item);


static inline KevItemSet* kev_lr_itemset_create(void) {
  KevItemSet* itemset = kev_itemset_pool_allocate();
  if (!itemset) return NULL;
  itemset->items = NULL;
  itemset->gotos = NULL;
  return itemset;
}

static inline KevItem* kev_lr_item_create(KevRule* rule, size_t dot) {
  KevItem* item = kev_item_pool_allocate();
  if (!item) return NULL;
  item->rule = rule;
  item->dot = dot;
  item->lookahead = NULL;
  return item;
}

static inline KevItem* kev_lr_item_create_copy(KevItem* item) {
  KevItem* ret = kev_item_pool_allocate();
  ret->rule = item->rule;
  ret->dot = item->dot;
  ret->lookahead = NULL;
  if (item->lookahead && !(ret->lookahead = kev_bitset_create_copy(item->lookahead))) {
    kev_lr_item_delete(ret);
    return NULL;
  }
  return ret;
}

static inline void kev_lr_item_delete(KevItem* item) {
  if (item) {
    kev_bitset_delete(item->lookahead);
    kev_item_pool_deallocate(item);
  }
}

static inline void kev_lr_itemset_add_goto(KevItemSet* itemset, KevItemSetGoto* go_to) {
  go_to->next = itemset->gotos;
  itemset->gotos = go_to;
}

static inline bool kev_lr_itemset_goto(KevItemSet* itemset, KevSymbol* symbol, KevItemSet* iset) {
  KevItemSetGoto* go_to = kev_itemsetgoto_pool_allocate();
  if (!go_to) return false;
  go_to->symbol = symbol;
  go_to->itemset = iset;
  kev_lr_itemset_add_goto(itemset, go_to);
  return true;
}

#endif
