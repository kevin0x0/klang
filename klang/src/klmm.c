#include "klang/include/klmm.h"

void klmm_destroy(KlMM* klmm) {
  klgc_clean_all(klmm->valuelist);
  klmm->valuelist = NULL;
  klmm->listlen = 0;
  klmm->limit = 0;
}
