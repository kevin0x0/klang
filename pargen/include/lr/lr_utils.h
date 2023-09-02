#ifndef KEVCC_PARGEN_INCLUDE_LR_LR_UTILS_H
#define KEVCC_PARGEN_INCLUDE_LR_LR_UTILS_H

#include "pargen/include/lr/collection.h"
#include "pargen/include/lr/hashmap/goto_map.h"

static inline void kev_lr_util_assign_itemset_id(KevItemSet** itemsets, size_t itemset_no);
bool kev_lr_util_generate_gotos(KevItemSet* itemset, KevItemSetClosure* closure, KevGotoMap* goto_container);
void kev_lr_util_compute_first(KevBitSet** firsts, KevSymbol* symbol, size_t epsilon);
KevBitSet** kev_lr_util_compute_first_array(KevSymbol** symbols, size_t symbol_no, size_t terminal_no);
void kev_lr_util_destroy_first_array(KevBitSet** firsts, size_t size);
KevSymbol* kev_lr_util_augment(KevSymbol* start);
KevBitSet* kev_lr_util_symbols_to_bitset(KevSymbol** symbols, size_t length);
size_t kev_lr_util_label_symbols(KevSymbol** symbols, size_t symbol_no);
KevItemSet* kev_lr_util_get_start_itemset(KevSymbol* start, KevSymbol** lookahead, size_t length);
KevSymbol** kev_lr_util_get_symbol_array(KevSymbol* start, KevSymbol** lookahead, size_t la_len, size_t* p_size);
void kev_lr_util_symbol_array_partition(KevSymbol** array, size_t size);

static inline void kev_lr_util_assign_itemset_id(KevItemSet** itemsets, size_t itemset_no) {
  for (size_t i = 0; i < itemset_no; ++i)
    itemsets[i]->id = i;
}

#endif
