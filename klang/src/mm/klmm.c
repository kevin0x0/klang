#include "klang/include/mm/klmm.h"

void klmm_destroy(KlMM* klmm) {
  klmm_gc_clean_all(klmm, &klmm->allgc);
  klmm->allgc.next = NULL;
  klmm->mem_used = 0;
  klmm->limit = 0;
}
