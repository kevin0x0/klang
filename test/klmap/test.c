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
void gctestmap(KlState* state);


int main(int argc, char** argv) {
  KlMM klmm;
  klmm_init(&klmm, 1024);
  KlState* state = klapi_new_state(&klmm);
  gctest(state);
  klmm_destroy(&klmm);
  return 0;
}

void gctest(KlState* state) {
  KlMM* klmm = klstate_getmm(state);
  klapi_pushnil(state, 1);
  KlClass* klclass = klclass_create(klmm, 10, KLOBJECT_DEFAULT_ATTROFF, NULL, NULL);
  klapi_setobj(state, -1, klclass, KL_CLASS);
  klmm->root = NULL;
  klapi_pushnil(state, 1);
  clock_t t = clock();
  for (size_t i = 0; i < 10000000; ++i) {
    KlValue val;
    klvalue_setint(&val, i);
    char keystr[40];
    sprintf(keystr, "%zu", i);
    klapi_setstring(state, -1, keystr);
    KlString* key = klapi_getstring(state, -1);
    klclass_newshared(klclass, klmm, key, &val);
  }
  int a = 0;
  // for (size_t i = 0; i < 1000000; ++i) {
  //   KlValue val;
  //   klvalue_setint(&val, i);
  //   char keystr[40];
  //   sprintf(keystr, "%zu", i);
  //   klapi_setstring(state, -1, keystr);
  //   KlString* key = klapi_getstring(state, -1);
  //   a += klclass_find(klclass, key) ? 1 : 0;
  // }
  fprintf(stderr, "%f\n", (clock() - t) / (float)CLOCKS_PER_SEC);
  fprintf(stderr, "memory used: %zu\n", klmm->mem_used);
  klmm->root = klmm_to_gcobj(state);
  fprintf(stderr, "gc %d times\n", a);
  // FILE* file = fopen("hash.txt", "w");
  // for (KlClassSlot* slot = klclass->slots; slot != klclass->slots + klclass->capacity; ++slot) {
  //   if (slot->key)
  //     fprintf(file, "%zu\n", klstring_hash(slot->key));
  // }
  // fclose(file);
}

void gctestmap(KlState* state) {
  KlMM* klmm = klstate_getmm(state);
  klapi_pushnil(state, 1);
  KlClass* klclass = klclass_create(klmm, 10, KLOBJECT_DEFAULT_ATTROFF, NULL, NULL);
  klapi_setobj(state, -1, klclass, KL_CLASS);
  klmm->root = NULL;
  klapi_pushnil(state, 1);
  KlMap* map = state->global;
  for (size_t i = 0; i < 1000000; ++i) {
    KlValue val;
    klvalue_setint(&val, i);
    char keystr[40];
    sprintf(keystr, "%zu", i);
    klapi_setstring(state, -1, keystr);
    KlString* key = klapi_getstring(state, -1);
    klmap_insertstring(map, key, &val);
  }
  clock_t t = clock();
  int a = 0;
  for (size_t i = 0; i < 1000000; ++i) {
    KlValue val;
    klvalue_setint(&val, i);
    char keystr[40];
    sprintf(keystr, "%zu", i);
    klapi_setstring(state, -1, keystr);
    KlString* key = klapi_getstring(state, -1);
    a += klmap_searchstring(map, key) ? 1 : 0;
  }
  fprintf(stderr, "%f\n", (clock() - t) / (float)CLOCKS_PER_SEC);
  fprintf(stderr, "memory used: %zu\n", klmm->mem_used);
  klmm->root = klmm_to_gcobj(state);
  fprintf(stderr, "gc %d times\n", a);
}
