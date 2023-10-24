#include "klang/include/klmm.h"
#include "klang/include/value/klmap.h"


int main(void) {
  KlMM klmm;
  klmm_init(&klmm, 10);
  while (true) {

    KlValue* mapval = klmm_alloc_map(&klmm);
    KlMap* map = mapval->value.map;

    for (size_t i = 0; i < 100; ++i) {
      KlValue* tmp = klmm_alloc_int(&klmm, i);
      klmap_insert_move(map, kstring_create("Hello"), tmp);
      klvalue_ref_decr(tmp);
    }

    klvalue_ref_decr(mapval);
  }

  klmm_destroy(&klmm);
  return 0;
}
