#include "klang/include/mm/klmm.h"
#include "klang/include/value/klmap.h"
#include "klang/include/value/klarray.h"
#include "klang/include/value/klstring.h"
#include "klang/include/vm/klstate.h"
#include <stdio.h>


int main(void) {
  while (true) {
    KlMM klmm;
    klmm_init(&klmm, 1024 * 1024);
    KlState* state = klstate_create(&klmm);
    klmm_register_root(&klmm, (KlGCObject*)state);
    
    char buf[100] = "hello";
    for (size_t cnt = 0; cnt < 10000000; ++cnt) {
      sprintf(buf, "hello %d\n", cnt);
      KlString* str = klstrpool_new_string(&state->strpool, buf);
      (void)str;
    }
    KlString* str1 = klstrpool_new_string(&state->strpool, buf);
    KlString* str2 = klstrpool_new_string(&state->strpool, buf);
    printf("%d\n", str1 == str2);
    printf("%d\n", state->strpool.size);
    KlString* iter = klstrpool_iter_begin(&state->strpool);
    while (iter != klstrpool_iter_end(&state->strpool)) {
      printf("%s\n", klstring_get_content(iter));
      iter = klstrpool_iter_next(iter);
    }
    klmm_destroy(&klmm);
  }
  return 0;
}
