#ifndef KEVCC_PARGEN_INCLUDE_LR_LR_UTILS_H
#define KEVCC_PARGEN_INCLUDE_LR_LR_UTILS_H

#include "pargen/include/lr/collection.h"
#include "pargen/include/lr/hashmap/goto_map.h"

static inline void kev_lrs_assign_itemset_id(KevItemSet** itemsets, size_t itemset_no);
bool kev_lrs_generate_gotos(KevItemSet* itemset, KevItemSetClosure* closure, KevGotoMap* goto_container);
void kev_lrs_compute_first(KevBitSet** firsts, KevSymbol* symbol, size_t epsilon);
KevBitSet** kev_lrs_compute_first_array(KevSymbol** symbols, size_t symbol_no, size_t terminal_no);
void kev_lrs_destroy_first_array(KevBitSet** firsts, size_t size);
KevSymbol* kev_lrs_augment(KevSymbol* start);
KevBitSet* kev_lrs_symbols_to_bitset(KevSymbol** symbols, size_t length);
size_t kev_lrs_label_symbols(KevSymbol** symbols, size_t symbol_no);
KevItemSet* kev_lrs_get_start_itemset(KevSymbol* start, KevSymbol** lookahead, size_t length);

static inline void kev_lrs_assign_itemset_id(KevItemSet** itemsets, size_t itemset_no) {
  for (size_t i = 0; i < itemset_no; ++i)
    itemsets[i]->id = i;
}

#endif
