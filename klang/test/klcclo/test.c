#include "klang/include/mm/klmm.h"
#include "klang/include/klapi.h"

#include <stdio.h>

KlException fibonacci(KlState* state);


int main(void) {
  KlMM klmm;
  klmm_init(&klmm, 1024);
  KlState* K = klapi_new_state(&klmm);

  KlInt val = 35;
  klapi_pushcfunc(K, fibonacci);
  klapi_pushint(K, val);
  KlException exception = klapi_call(K, klapi_access(K, -2), 1, 1);
  if (exception) {
    fprintf(stderr, "exception occurred\n");
    fprintf(stderr, "%s\n", K->throwinfo.exception.message);
    return -1;
  }
  printf("fibonacci(%lld) = %lld\n", val, klvalue_getint(klapi_access(K, -1)));

  //for (int i = 0; i < 100; ++i) {
  //  klapi_pushobj(K, klstate_create(&klmm, global, strpool), KL_STATE);
  //}
  //klapi_pop(K, 100);

  //klapi_pop(K, 2);

  //klapi_pushmap(K, 3);
  //klapi_pushstring(K, "Hello, world!");
  //klapi_pushcfunc(K, fibonacci);
  //KlMap* map = klapi_getmap(K, -3);
  //KlValue* key = klapi_access(K, -1);
  //KlValue* value = klapi_access(K, -2);
  //klmap_insert(map, key, value);
  //KlMapIter itr = klmap_search(map, key);
  //printf("%s\n", klstring_content((KlString*)itr->value.value.gcobj));
  //klmm_do_gc(&klmm);

  //klapi_pusharray(K, 0);
  //KlArray* array = klvalue_getobj(klapi_access(K, -1), KlArray*);
  //for (size_t i = 0; i < 100; ++i) {
  //  KlValue square;
  //  klvalue_setint(&square, i * i);
  //  klarray_push_back(array, &square);
  //}

  //for (KlArrayIter iter = klarray_iter_begin(array); iter != klarray_iter_end(array); ++iter) {
  //  fprintf(stdout, "%d\n", klvalue_getint(iter));
  //}

  klmm_destroy(&klmm);
  return 0;
}

KlException fibonacci(KlState* state) {
  if (klstate_getnarg(state) != 1)
    return KL_E_ARGNO;

  KlInt val = klvalue_getint(klapi_access(state, -1));
  if (val <= 1) {
    klapi_setint(state, -1, val);
  } else {
    klapi_pushint(state, val - 1);
    KlValue callable;
    klvalue_setcfunc(&callable, fibonacci);
    KlException exception = klapi_call(state, &callable, 1, 1);
    if (exception)
      return exception;
    val = klvalue_getint(klapi_access(state, -2));
    klapi_pushint(state, val - 2);
    exception = klapi_call(state, &callable, 1, 1);
    if (exception)
      return exception;
    KlInt ret1 = klvalue_getint(klapi_access(state, -2));
    KlInt ret2 = klvalue_getint(klapi_access(state, -1));
    klapi_pop(state, 2);
    klapi_setint(state, -1, ret1 + ret2);
  }
  return KL_E_NONE;
}
