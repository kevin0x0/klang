#include "kevlr/include/collection.h"

#include <stdlib.h>


void kev_lr_collection_delete(KevLRCollection* collec) {
  if (!collec) return;
  free(collec->symbols);
  for (size_t i = 0; i < collec->symbol_no; ++i) {
    kev_bitset_delete(collec->firsts[i]);
  }
  free(collec->firsts);
  for (size_t i = 0; i < collec->itemset_no; ++i) {
    kev_lr_itemset_delete(collec->itemsets[i]);
  }
  free(collec->itemsets);
  kev_lr_rule_delete(collec->start_rule);
  kev_lr_symbol_delete(collec->start);
  free(collec);
}
