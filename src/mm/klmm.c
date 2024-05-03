#include "include/mm/klmm.h"

void klmm_destroy(KlMM* klmm) {
  klmm_gc_clean_all(klmm, klmm->allgc);
  klmm->allgc = NULL;
  kl_assert(klmm->mem_used == 0, "");
  klmm->limit = 0;
}
