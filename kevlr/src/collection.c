#include "kevlr/include/collection.h"

#include <stdlib.h>


void klr_collection_delete(KlrCollection* collec) {
  if (k_unlikely(!collec)) return;
  free(collec->symbols);
  for (size_t i = 0; i < collec->symbol_no; ++i) {
    kbitset_delete(collec->firsts[i]);
  }
  free(collec->firsts);
  for (size_t i = 0; i < collec->itemset_no; ++i) {
    klr_itemset_delete(collec->itemsets[i]);
  }
  free(collec->itemsets);
  klr_rule_delete(collec->start_rule);
  klr_symbol_delete(collec->start);
  free(collec);
}
