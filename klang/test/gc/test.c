#include "klang/include/mm/klgcobject.h"
#include "klang/include/mm/klmm.h"
#include "klang/include/value/klmap.h"
#include "klang/include/value/klarray.h"
#include <stdio.h>


int main(void) {
  while (true) {
    KlMM klmm;
    klmm_init(&klmm, 1024 * 1024 * 1024);
    KlArray* array = klarray_create(&klmm, 32);
    klmm_register_root(&klmm, (KlGCObject*)array);
    char buf[100] = "hello";
    
    for (size_t cnt = 0; cnt < 100000; ++cnt) {
      klarray_make_empty(array);
      KlMap* map = klmap_create(&klmm, 4);
      KlValue tmp;
      tmp.type = KL_MAP;
      tmp.value.gcobj = (KlGCObject*)map;
      klarray_push_back(array, &tmp);
      for (size_t i = 0; i < 16; ++i) {
        sprintf(buf, "hello %d", i);
        tmp.type = KL_STRING;
        tmp.value.gcobj = (KlGCObject*)klstring_create(&klmm, buf);
        klarray_push_back(array, &tmp);
      }
      for (size_t i = 0; i < 14; i += 2) {
        tmp.type = KL_STRING;
        tmp.value.gcobj = klarray_access(array, i + 2)->value.gcobj;
        klmap_insert(map, klarray_access(array, i + 1)->value.gcobj, &tmp);
      }
      // for (KlMapIter itr = klmap_iter_begin(map); itr != klmap_iter_end(map); itr = klmap_iter_next(itr)) {
      //   klstring_print((KlString*)itr->key, buf);
      //   printf("%s : ", buf);
      //   klstring_print((KlString*)itr->value.value.gcobj, buf);
      //   printf("%s\n", buf);
      // }
    }
    // for (KlMapIter itr = klmap_iter_begin(map); itr != klmap_iter_end(map); itr = klmap_iter_next(itr))
    //   printf("%s %d\n", kstring_get_content(itr->key), itr->value.value.intval);


    klmm_destroy(&klmm);
  }
  return 0;
}
