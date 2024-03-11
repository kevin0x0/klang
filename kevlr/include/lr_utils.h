#ifndef KEVCC_KEVLR_INCLUDE_LR_UTILS_H
#define KEVCC_KEVLR_INCLUDE_LR_UTILS_H

#include "kevlr/include/collection.h"
#include "kevlr/include/set/transset.h"

static inline void klr_util_label_itemsets(KlrItemSet** itemsets, size_t nitemset);
bool klr_util_generate_transition(KlrItemPoolCollec* pool, KlrItemSet* itemset, KlrItemSetClosure* closure, KlrTransSet* transitions);
void klr_util_destroy_terminal_set_array(KBitSet** array, size_t size);
KlrSymbol* klr_util_augment(KlrSymbol* start);
KBitSet* klr_util_symbols_to_bitset(KlrSymbol** symbols, size_t length);
static inline void klr_util_label_symbols(KlrSymbol** symbols, size_t nsymbol);
KlrItemSet* klr_util_get_start_itemset(KlrItemPoolCollec* pool, KlrSymbol* start, KlrSymbol** lookahead, size_t length);
KlrSymbol** klr_util_get_symbol_array_with_index_unchanged(KlrSymbol* start, KlrSymbol** ends, size_t nend, size_t* p_size);
KlrSymbol** klr_util_get_symbol_array(KlrSymbol* start, KlrSymbol** end_symbols, size_t nend, size_t* p_size);
size_t klr_util_symbol_array_partition(KlrSymbol** array, size_t size);
KBitSet** klr_util_compute_firsts(KlrSymbol** symbols, size_t nsymbol, size_t nterminal);
KBitSet** klr_util_compute_follows(KlrSymbol** symbols, KBitSet** firsts, size_t nsymbol, size_t nterminal, KlrSymbol* start, KlrSymbol** end_symbols, size_t nend);
size_t klr_util_user_symbol_max_id(KlrCollection* collec);
size_t klr_util_user_terminal_max_id(KlrCollection* collec);
size_t klr_util_user_nonterminal_max_id(KlrCollection* collec);

static inline void klr_util_label_itemsets(KlrItemSet** itemsets, size_t nitemset) {
  for (size_t i = 0; i < nitemset; ++i)
    itemsets[i]->id = i;
}

static inline void klr_util_label_symbols(KlrSymbol** symbols, size_t nsymbol) {
  for (size_t i = 0; i < nsymbol; ++i)
    symbols[i]->index = i;
}

#endif
