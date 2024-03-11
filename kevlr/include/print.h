#ifndef KEVCC_KEVLR_INCLUDE_PRINT_H
#define KEVCC_KEVLR_INCLUDE_PRINT_H

#include "kevlr/include/collection.h"
#include "kevlr/include/table.h"

#include <stdio.h>

typedef struct tagKlrPrintConfig {
  const char* unnamed;
} KlrPrintConfig;

static inline void klr_print_item(FILE* out, KlrCollection* collec, KlrItem* item);
bool klr_print_itemset(FILE* out, KlrCollection* collec, KlrItemSet* itemset, bool print_closure);
void klr_print_itemset_with_closure(FILE* out, KlrCollection* collec, KlrItemSet* itemset, KlrItemSetClosure* closure);
bool klr_print_collection(FILE* out, KlrCollection* collec, bool print_closure);
bool klr_print_symbols(FILE* out, KlrCollection* collec);

void klr_print_rule(FILE* out, KlrRule* rule);

void klr_print_trans_table(FILE* out, KlrTable* table);
void klr_print_action_table(FILE* out, KlrTable* table);


void klr_print_kernel_item(FILE* out, KlrCollection* collec, KlrItem* kitem);
void klr_print_non_kernel_item(FILE* out, KlrCollection* collec, KlrRule* rule, KBitSet* lookahead);
void klr_print_terminal_set(FILE* out, KlrCollection* collec, KBitSet* lookahead);


static inline void klr_print_item(FILE* out, KlrCollection* collec, KlrItem* item) {
  klr_print_kernel_item(out, collec, item);
}

#endif
