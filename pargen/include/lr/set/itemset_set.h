#ifndef KEVCC_UTILS_INCLUDE_SET_ITEMSET_SET_H
#define KEVCC_UTILS_INCLUDE_SET_ITEMSET_SET_H

#include "pargen/include/lr/item.h"
#include "utils/include/general/global_def.h"

typedef struct tagKevItemSetSetNode {
  KevItemSet* element;
  struct tagKevItemSetSetNode* next;
} KevItemSetSetNode;

typedef struct tagKevItemSetSet {
  KevItemSetSetNode** array;
  bool (*equal)(KevItemSet*, KevItemSet*);
  size_t capacity;
  size_t size;
} KevItemSetSet;

bool kev_itemsetset_init(KevItemSetSet* set, size_t capacity, bool (*equal)(KevItemSet*, KevItemSet*));
void kev_itemsetset_destroy(KevItemSetSet* set);
KevItemSetSet* kev_itemsetset_create(size_t capacity, bool (*equal)(KevItemSet*, KevItemSet*));
void kev_itemsetset_delete(KevItemSetSet* set);

bool kev_itemsetset_insert(KevItemSetSet* set, KevItemSet* element);
KevItemSetSetNode* kev_itemsetset_search(KevItemSetSet* set, KevItemSet* element);

#endif
