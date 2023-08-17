#ifndef KEVCC_PARGEN_INCLUDE_LR_LR_H
#define KEVCC_PARGEN_INCLUDE_LR_LR_H

#include "pargen/include/lr/item.h"
#include "utils/include/array/addr_array.h"
#include "utils/include/set/bitset.h"

#define KEV_LR_ACTION_ERR     (0)
#define KEV_LR_ACTION_SHI     (1)
#define KEV_LR_ACTION_RED     (2)
#define KEV_LR_ACTION_ACC     (3)

#define KEV_LR_GOTO_NONE      ((KevLRGotoEntry)-1)

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
  KevLRActionEntry** table;
  size_t itemset_no;
  size_t symbol_no;
} KevLRAction;

typedef int64_t KevLRGotoEntry;
typedef struct tagKevLRGoto {
  KevLRGotoEntry** table;
  size_t itemset_no;
  size_t symbol_no;
} KevLRGoto;

/* generation of lr collection */
KevLRCollection* kev_lr_collection_create_lalr(KevSymbol* start, KevSymbol** lookahead, size_t la_len);
KevLRCollection* kev_lr_collection_create_lr1(KevSymbol* start, KevSymbol** lookahead, size_t la_len);
KevLRCollection* kev_lr_collection_create_slr(KevSymbol* start, KevSymbol** lookahead, size_t la_len);
void kev_lr_collection_delete(KevLRCollection* collec);

/* generation of GOTO and ACTION table */
KevLRAction* kev_lr_action_create(KevLRCollection* collec, KevLRGoto* goto_table);
KevLRGoto* kev_lr_goto_create(KevLRCollection* collec);
void kev_lr_action_delete(KevLRAction* table);
void kev_lr_goto_delete(KevLRGoto* table);

/* get */
static inline KevItemSet* kev_lr_get_itemset_by_index(KevLRCollection* collec, size_t index);
static inline KevBitSet* kev_lr_get_first_by_index(KevLRCollection* collec, size_t index);
static inline size_t kev_lr_get_itmeset_no(KevLRCollection* collec);
static inline size_t kev_lr_get_symbol_no(KevLRCollection* collec);
static inline size_t kev_lr_get_terminal_no(KevLRCollection* collec);

/* general function */
void kev_lr_compute_first(KevBitSet** firsts, KevSymbol* symbol, size_t epsilon);
KevBitSet** kev_lr_compute_first_array(KevSymbol** symbols, size_t symbol_no, size_t terminal_no);
void kev_lr_destroy_first_array(KevBitSet** firsts, size_t size);
KevSymbol* kev_lr_augment(KevSymbol* start);
KevBitSet* kev_lr_symbols_to_bitset(KevSymbol** symbols, size_t length);
size_t kev_lr_label_symbols(KevSymbol** symbols, size_t symbol_no);
KevItemSet* kev_lr_get_start_itemset(KevSymbol* start, KevSymbol** lookahead, size_t length);

bool kev_lr_closure(KevItemSet* itemset, KevAddrArray* closure, KevBitSet** la_symbols, KevBitSet** firsts, size_t epsilon);
bool kev_lr_closure_create(KevLRCollection* collec, KevItemSet* itemset, KevAddrArray** p_closure, KevBitSet*** p_la_symbols);
void kev_lr_closure_make_empty(KevAddrArray* closure, KevBitSet** la_symbols);
void kev_lr_closure_destroy(KevAddrArray* closure, KevBitSet** la_symbols);
void kev_lr_closure_delete(KevAddrArray* closure, KevBitSet** la_symbols);
KevBitSet* kev_lr_get_kernel_item_follows(KevKernelItem* kitem, KevBitSet** firsts, size_t epsilon);
KevBitSet* kev_lr_get_non_kernel_item_follows(KevRule* rule, KevBitSet* lookahead, KevBitSet** firsts, size_t epsilon);

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
