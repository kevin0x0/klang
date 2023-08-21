#ifndef KEVCC_PARGEN_INCLUDE_LR_PRINT_H
#define KEVCC_PARGEN_INCLUDE_LR_PRINT_H

#include "pargen/include/lr/collection.h"

#include <stdio.h>

bool kev_lr_print_itemset(FILE* out, KevLRCollection* collec, KevItemSet* itemset, bool print_closure);
bool kev_lr_print_collection(FILE* out, KevLRCollection* collec, bool print_closure);

void kev_lr_print_kernel_item(FILE* out, KevLRCollection* collec, KevItem* kitem);
void kev_lr_print_non_kernel_item(FILE* out, KevLRCollection* collec, KevRule* rule, KevBitSet* lookahead);
void kev_lr_print_terminal_set(FILE* out, KevLRCollection* collec, KevBitSet* lookahead);

#endif
