#include "klang/include/klapi.h"
#include "klang/include/klstate.h"
#include "klang/include/value/klarray.h"
#include <stdio.h>

int main(int argc, char *argv[]) {
  while (true) {
    KlState* klstate = klstate_create();

    klstate_push_map(klstate);
    for (size_t i = 0; i < 100; ++i) {
      klstate_push_int(klstate, i);
      char buf[32];
      sprintf(buf, "%d", (int)i);
      KString* hello = kstring_create("hello");
      KString* num = kstring_create(buf);
      KString* key = kstring_concat(hello, num);
      kstring_delete(hello);
      kstring_delete(num);
      klstate_push_string_move(klstate, key);
      if (!klstate_map_insert(klstate, 3, 1, 2)) {
        fprintf(stderr, "insertion failed(%d)\n", (int)i);
      }
      klstate_multipop(klstate, 2);
    }

    //KlMap* map = (KlMap*)klarray_access_from_top(klstate->stack, 1)->value.gcobj;
    //KlMapIter itr = klmap_iter_begin(map);
    //while (itr != klmap_iter_end(map)) {
    //  printf(".%s = %d\n", kstring_get_content(itr->key), itr->value.value.intval);
    //  itr = klmap_iter_next(itr);
    //}

    klstate_delete(klstate);
  }
  return 0;
}
