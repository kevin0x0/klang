#ifndef KEVCC_PARGEN_INCLUDE_LR_COLLECTION_H
#define KEVCC_PARGEN_INCLUDE_LR_COLLECTION_H

#include "pargen/include/lr/item.h"
#include "utils/include/array/addr_array.h"

typedef struct tagKevLRCollection {
  KevSymbol** symbols;
  size_t symbol_no;
  size_t terminal_no;
  KevItemSet** itemsets;
  size_t itemset_no;
  KevBitSet** firsts;
  KevSymbol* start;
  KevRule* start_rule;
} KevLRCollection;

/* generation of lr collection */
KevLRCollection* kev_lr_collection_create_lalr(KevSymbol* start, KevSymbol** lookahead, size_t la_len);
KevLRCollection* kev_lr_collection_create_lr1(KevSymbol* start, KevSymbol** lookahead, size_t la_len);
KevLRCollection* kev_lr_collection_create_slr(KevSymbol* start, KevSymbol** lookahead, size_t la_len);

void kev_lr_collection_delete(KevLRCollection* collec);

/* get methods */
static inline KevItemSet* kev_lr_get_itemset_by_index(KevLRCollection* collec, size_t index);
static inline KevBitSet* kev_lr_get_first_by_index(KevLRCollection* collec, size_t index);
static inline size_t kev_lr_get_itmeset_no(KevLRCollection* collec);
static inline size_t kev_lr_get_symbol_no(KevLRCollection* collec);
static inline size_t kev_lr_get_terminal_no(KevLRCollection* collec);


static inline KevItemSet* kev_lr_get_itemset_by_index(KevLRCollection* collec, size_t index) {
  return collec->itemsets[index];
}

static inline KevBitSet* kev_lr_get_first_by_index(KevLRCollection* collec, size_t index) {
  return collec->firsts[index];
}

static inline size_t kev_lr_get_itmeset_no(KevLRCollection* collec) {
  return collec->itemset_no;
}

static inline size_t kev_lr_get_symbol_no(KevLRCollection* collec) {
  return collec->symbol_no;
}

static inline size_t kev_lr_get_terminal_no(KevLRCollection* collec) {
  return collec->terminal_no;
}

#endif
