#ifndef KEVCC_KEVLR_INCLUDE_LR_UTILS_H
#define KEVCC_KEVLR_INCLUDE_LR_UTILS_H

#include "kevlr/include/collection.h"
#include "kevlr/include/hashmap/trans_map.h"

static inline void klr_util_assign_itemset_id(KlrItemSet** itemsets, size_t itemset_no);
bool klr_util_generate_transition(KlrItemSet* itemset, KlrItemSetClosure* closure, KlrTransMap* transitions);
void klr_util_destroy_terminal_set_array(KBitSet** array, size_t size);
KlrSymbol* klr_util_augment(KlrSymbol* start);
KBitSet* klr_util_symbols_to_bitset(KlrSymbol** symbols, size_t length);
static inline void klr_util_label_symbols(KlrSymbol** symbols, size_t symbol_no);
KlrItemSet* klr_util_get_start_itemset(KlrSymbol* start, KlrSymbol** lookahead, size_t length);
KlrSymbol** klr_util_get_symbol_array_without_changing_index(KlrSymbol* start, KlrSymbol** ends, size_t ends_no, size_t* p_size);
KlrSymbol** klr_util_get_symbol_array(KlrSymbol* start, KlrSymbol** ends, size_t ends_no, size_t* p_size);
size_t klr_util_symbol_array_partition(KlrSymbol** array, size_t size);
KBitSet** klr_util_compute_firsts(KlrSymbol** symbols, size_t symbol_no, size_t terminal_no);
KBitSet** klr_util_compute_follows(KlrSymbol** symbols, KBitSet** firsts, size_t symbol_no, size_t terminal_no, KlrSymbol* start, KlrSymbol** ends, size_t ends_no);
size_t klr_util_user_symbol_max_id(KlrCollection* collec);
size_t klr_util_user_terminal_max_id(KlrCollection* collec);
size_t klr_util_user_nonterminal_max_id(KlrCollection* collec);

static inline void klr_util_assign_itemset_id(KlrItemSet** itemsets, size_t itemset_no) {
  for (size_t i = 0; i < itemset_no; ++i)
    itemsets[i]->id = i;
}

static inline void klr_util_label_symbols(KlrSymbol** symbols, size_t symbol_no) {
  for (size_t i = 0; i < symbol_no; ++i)
    symbols[i]->index = i;
}

#endif
