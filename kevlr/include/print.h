#ifndef KEVCC_KEVLR_INCLUDE_PRINT_H
#define KEVCC_KEVLR_INCLUDE_PRINT_H

#include "kevlr/include/collection.h"
#include "kevlr/include/table.h"

#include <stdio.h>

static inline void kev_lr_print_item(FILE* out, KevLRCollection* collec, KevItem* item);
bool kev_lr_print_itemset(FILE* out, KevLRCollection* collec, KevItemSet* itemset, bool print_closure);
void kev_lr_print_itemset_with_closure(FILE* out, KevLRCollection* collec, KevItemSet* itemset, KevItemSetClosure* closure);
bool kev_lr_print_collection(FILE* out, KevLRCollection* collec, bool print_closure);
bool kev_lr_print_symbols(FILE* out, KevLRCollection* collec);

void kev_lr_print_rule(FILE* out, KevRule* rule);

void kev_lr_print_goto_table(FILE* out, KevLRTable* table);
void kev_lr_print_action_table(FILE* out, KevLRTable* table);


void kev_lr_print_kernel_item(FILE* out, KevLRCollection* collec, KevItem* kitem);
void kev_lr_print_non_kernel_item(FILE* out, KevLRCollection* collec, KevRule* rule, KevBitSet* lookahead);
void kev_lr_print_terminal_set(FILE* out, KevLRCollection* collec, KevBitSet* lookahead);


static inline void kev_lr_print_item(FILE* out, KevLRCollection* collec, KevItem* item) {
  kev_lr_print_kernel_item(out, collec, item);
}

#endif
