#ifndef KEVCC_PARGEN_INCLUDE_LR_LR_H
#define KEVCC_PARGEN_INCLUDE_LR_LR_H

#include "pargen/include/lr/item.h"
#include "utils/include/array/addr_array.h"
#include "utils/include/set/bitset.h"

typedef struct tagKevLRCollection {
  KevSymbol** symbols;
  size_t symbol_no;
  size_t terminal_no;
  KevItemSet** itemsets;
  size_t itemset_no;
  KevBitSet** firsts;
} KevLRCollection;

typedef struct tagKevLRActionEntry {
  size_t info;
  int action;
} KevLRActionEntry;

typedef struct tagKevLRAction {
  KevLRActionEntry** action;
  size_t itemset_no;
  size_t symbol_no;
} KevLRAction;

/* generation of lr collection */
KevLRCollection* kev_create_lalr_collection(KevSymbol* start, KevSymbol** lookahead, size_t la_len);
KevLRCollection* kev_create_lr0_collection(KevSymbol* start, KevSymbol** lookahead, size_t la_len);
KevLRCollection* kev_create_lr1_collection(KevSymbol* start, KevSymbol** lookahead, size_t la_len);
KevLRCollection* kev_create_slr_collection(KevSymbol* start, KevSymbol** lookahead, size_t la_len);

void kev_lr_delete_collection(KevLRCollection* collec);

/* get */
static inline KevItemSet* kev_lr_get_itemset_by_index(KevLRCollection* collec, size_t index);
static inline KevBitSet* kev_lr_get_first_by_index(KevLRCollection* collec, size_t index);
static inline size_t kev_lr_get_itmeset_no(KevLRCollection* collec);
static inline size_t kev_lr_get_symbol_no(KevLRCollection* collec);
static inline size_t kev_lr_get_terminal_no(KevLRCollection* collec);

/* general computation */
KevAddrArray* kev_lr_closure(KevLRCollection* collec, KevItemSet* itemset, KevBitSet*** p_la_symbols);
void kev_lr_delete_closure(KevAddrArray* closure, KevBitSet** la_symbols);
KevBitSet* kev_lr_get_kernel_item_follows(KevLRCollection* collec, KevKernelItem* kitem);
KevBitSet* kev_lr_get_non_kernel_item_follows(KevLRCollection* collec, KevRule* rule, KevBitSet* lookahead);

/* general function */
void kev_lr_compute_first(KevBitSet** firsts, KevSymbol* symbol, size_t epsilon);
KevBitSet** kev_lr_compute_first_array(KevSymbol** symbols, size_t symbol_no, size_t terminal_no);
void kev_lr_destroy_first_array(KevBitSet** firsts, size_t size);
KevSymbol* kev_lr_augment(KevSymbol* start);
KevBitSet* kev_lr_symbols_to_bitset(KevSymbol** symbols, size_t length);
size_t kev_lr_label_symbols(KevSymbol** symbols, size_t symbol_no);
KevItemSet* kev_lr_get_start_itemset(KevSymbol* start, KevSymbol** lookahead, size_t length);

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
