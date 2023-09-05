#ifndef KEVCC_KEVLR_INCLUDE_LR_UTILS_H
#define KEVCC_KEVLR_INCLUDE_LR_UTILS_H

#include "kevlr/include/collection.h"
#include "kevlr/include/hashmap/goto_map.h"

static inline void kev_lr_util_assign_itemset_id(KevItemSet** itemsets, size_t itemset_no);
bool kev_lr_util_generate_gotos(KevItemSet* itemset, KevItemSetClosure* closure, KevGotoMap* goto_container);
void kev_lr_util_destroy_terminal_set_array(KevBitSet** array, size_t size);
KevSymbol* kev_lr_util_augment(KevSymbol* start);
KevBitSet* kev_lr_util_symbols_to_bitset(KevSymbol** symbols, size_t length);
size_t kev_lr_util_label_symbols(KevSymbol** symbols, size_t symbol_no);
KevItemSet* kev_lr_util_get_start_itemset(KevSymbol* start, KevSymbol** lookahead, size_t length);
KevSymbol** kev_lr_util_get_symbol_array(KevSymbol* start, KevSymbol** ends, size_t ends_no, size_t* p_size);
void kev_lr_util_symbol_array_partition(KevSymbol** array, size_t size);
KevBitSet** kev_lr_util_compute_firsts(KevSymbol** symbols, size_t symbol_no, size_t terminal_no);
KevBitSet** kev_lr_util_compute_follows(KevSymbol** symbols, KevBitSet** firsts, size_t symbol_no, size_t terminal_no, KevSymbol* start, KevSymbol** ends, size_t ends_no);
size_t kev_lr_util_symbol_max_id(KevLRCollection* collec);

static inline void kev_lr_util_assign_itemset_id(KevItemSet** itemsets, size_t itemset_no) {
  for (size_t i = 0; i < itemset_no; ++i)
    itemsets[i]->id = i;
}

#endif
