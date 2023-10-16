#include "kevlr/include/set/transset.h"

#include <stdlib.h>
#include <string.h>

bool klr_transset_init(KlrTransSet* transset, size_t symbol_no) {
  if (!transset) return false;
  transset->targets = NULL;
  if (!karray_init(&transset->symbols))
    return false;
  transset->targets = (KlrItemSet**)malloc(sizeof (KlrItemSet*) * symbol_no);
  if (!transset->targets) {
    karray_destroy(&transset->symbols);
    return false;
  }
  memset(transset->targets, 0, sizeof (KlrItemSet*) * symbol_no);
  return true;
}

KlrTransSet* klr_transset_create(size_t symbol_no) {
  KlrTransSet* set = (KlrTransSet*)malloc(sizeof(KlrTransSet));
  if (!set || !klr_transset_init(set, symbol_no)) {
    free(set);
    return NULL;
  }
  return set;
}

void klr_transset_destroy(KlrTransSet* transset) {
  if (!transset) return;
  karray_destroy(&transset->symbols);
  free(transset->targets);
}

void klr_transset_delete(KlrTransSet* transset) {
  klr_transset_destroy(transset);
  free(transset);
}

