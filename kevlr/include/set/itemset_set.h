#ifndef KEVCC_KEVLR_INCLUDE_SET_ITEMSET_SET_H
#define KEVCC_KEVLR_INCLUDE_SET_ITEMSET_SET_H

#include "kevlr/include/itemset_def.h"
#include "utils/include/utils/utils.h"

typedef struct tagKlrItemSetSetNode {
  KlrItemSet* element;
  size_t hashval;
  struct tagKlrItemSetSetNode* next;
} KlrItemSetSetNode;

typedef struct tagKlrItemSetSet {
  KlrItemSetSetNode** array;
  bool (*equal)(KlrItemSet*, KlrItemSet*);
  size_t capacity;
  size_t size;
} KlrItemSetSet;

bool klr_itemsetset_init(KlrItemSetSet* set, size_t capacity, bool (*equal)(KlrItemSet*, KlrItemSet*));
void klr_itemsetset_destroy(KlrItemSetSet* set);
KlrItemSetSet* klr_itemsetset_create(size_t capacity, bool (*equal)(KlrItemSet*, KlrItemSet*));
void klr_itemsetset_delete(KlrItemSetSet* set);

bool klr_itemsetset_insert(KlrItemSetSet* set, KlrItemSet* element);
KlrItemSetSetNode* klr_itemsetset_search(KlrItemSetSet* set, KlrItemSet* element);

#endif
