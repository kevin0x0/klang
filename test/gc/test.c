#include "include/klapi.h"
#include "include/mm/klmm.h"
#include "include/value/klcfunc.h"
#include "include/value/klclass.h"
#include "include/value/klclosure.h"
#include "include/value/klcoroutine.h"
#include "include/value/klmap.h"
#include "include/value/klstring.h"
#include "include/value/klvalue.h"
#include "include/lang/klinst.h"
#include <stdio.h>
#include <time.h>

void gctest(KlState* state);
void gctest0(KlState* state);
void gctest1(KlState* state);
void gctest2(KlState* state);


int a = 0;
size_t r = 0;
int main(int argc, char** argv) {
  if (argv[1] == NULL) return 1;
  r = atoi(argv[1]);
  KlMM klmm;
  klmm_init(&klmm, 1024);
  KlState* state = klapi_new_state(&klmm);
  gctest1(state);
  gctest0(state);
  a = 0;
  //gctest2(state);
  klmm_destroy(&klmm);
  return 0;
}

void gctest1(KlState* state) {
  clock_t t = clock();
  KlMM* klmm = klstate_getmm(state);
  klapi_pushnil(state, 1);
  KlClass* klclass = klclass_create(klmm, 10, KLOBJECT_DEFAULT_ATTROFF, NULL, NULL);
  klapi_setobj(state, -1, klclass, KL_CLASS);
  //klmm->root = NULL;
  for (size_t i = 0; i < 9 * r; ++i) {
    char key[40];
    sprintf(key, "key%zu", i);
    klapi_pushstring(state, key);
    KlString* str = klapi_getstring(state, -1);
    klapi_storeglobal(state, str);
    //klclass_newshared(klclass, klmm, str, klapi_access(state, -1));
    klapi_pop(state, 1);
  }
  fprintf(stderr, "%f\n", (clock() - t) / (float)CLOCKS_PER_SEC);
  fprintf(stderr, "memory used: %zu\n", klmm->mem_used);
  t = clock();
  klmm->root = klmm_to_gcobj(state);
  //KlMapIter end = klmap_iter_end(state->global);
  //int a = 0;
  //for (KlMapIter itr = klmap_iter_begin(state->global); itr != end; itr = itr->next) {
  //  a++;
  //}
  for (size_t i = 0; i < 1; ++i) {
    klmm_do_gc(klmm);
  }
  fprintf(stderr, "%f\n", (clock() - t) / (float)CLOCKS_PER_SEC);
  fprintf(stderr, "gc %d times\n", a);
}

void gctest(KlState* state) {
  clock_t t = clock();
  KlMM* klmm = klstate_getmm(state);
  //klmm->root = NULL;
  for (size_t i = 0; i < 100000000; ++i) {
    char key[40];
    sprintf(key, "key%zu", i);
    klapi_pushstring(state, key);
    klapi_pop(state, 1);
    //KlString* str = klapi_getstring(state, -1);
    //klapi_storeglobal(state, str);
  }
  fprintf(stderr, "%f\n", (clock() - t) / (float)CLOCKS_PER_SEC);
  fprintf(stderr, "memory used: %zu\n", klmm->mem_used);
  t = clock();
  klmm->root = klmm_to_gcobj(state);
  for (size_t i = 0; i < 1; ++i) {
    klmm_do_gc(klmm);
  }
  fprintf(stderr, "%f\n", (clock() - t) / (float)CLOCKS_PER_SEC);
  fprintf(stderr, "gc %d times\n", a);
}

void gctest0(KlState* state) {
  clock_t t = clock();
  KlMM* klmm = klstate_getmm(state);
  //klmm->root = NULL;
  KlInstruction* code = (KlInstruction*)klmm_alloc(klmm, 100 * sizeof (KlInstruction));
  KlKFunction* kfunc = klkfunc_alloc(klmm, code, 100, 2, 0, 0, 100, 0);
  // KlRefInfo* refinfo = klkfunc_refinfo(kfunc);
  KlValue* constants = klkfunc_constants(kfunc);
  klapi_pushint(state, 10000 * r);
  klvalue_setvalue(&constants[0], klapi_access(state, -1));
  klapi_setstring(state, -1, "name");
  klvalue_setvalue(&constants[1], klapi_access(state, -1));
  code[0] = klinst_adjustargs();
  code[1] = klinst_loadi(0, 0);
  code[2] = klinst_loadc(1, 0);
  code[3] = klinst_loadnil(2, 0);
  code[4] = klinst_iforprep(0, 4);
  code[5] = klinst_loadc(3, 1);
  code[6] = klinst_move(4, 0);
  code[7] = klinst_concat(3, 3, 4);
  code[8] = klinst_iforloop(0, -4);
  code[9] = klinst_return1(0);
  // klapi_pushnil(state, 1);
  KlKClosure* kclo = klkclosure_create(klmm, kfunc, klapi_access(state, -1), &state->reflist, NULL);
  klkfunc_initdone(klmm, kfunc);
  klapi_setobj(state, -1, kclo, KL_KCLOSURE);
  //fprintf(stderr, "%f\n", (clock() - t) / (float)CLOCKS_PER_SEC);
  // klreflist_close(&state->reflist, klstate_getval(state, -1), klstate_getmm(state));
  // klapi_storeglobal(state, klstrpool_new_string(state->strpool, "fibonacci"));
  KlException exception = klapi_call(state, klapi_access(state, -1), 1, 1);
  fprintf(stderr, "%f\n", (clock() - t) / (float)CLOCKS_PER_SEC);
  fprintf(stderr, "memory used: %zu\n", klmm->mem_used);
  if (exception) {
    fprintf(stderr, "%s\n", state->throwinfo.exception.message);
  }
  klmm->root = klmm_to_gcobj(state);
  t = clock();
  klmm_do_gc(klmm);
  fprintf(stderr, "%f\n", (clock() - t) / (float)CLOCKS_PER_SEC);
  fprintf(stderr, "gc %d times\n", a);
}

void gctest2(KlState* state) {
  clock_t t = clock();
  KlMM* klmm = klstate_getmm(state);
  klapi_pushstring(state, "name");
  KlString* name = klapi_getstring(state, -1);
  for (size_t i = 0; i < 100000000; ++i) {
    char key[100];
    sprintf(key, "%zu", i);
    KlString* res = klstrpool_string_concat_cstyle(state->strpool, klstring_content(name), key);
  }
}

