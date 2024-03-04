#include "klang/include/klapi.h"
#include "klang/include/value/klclosure.h"
#include "klang/include/vm/klinst.h"
#include <stdio.h>
#include <time.h>



extern void* expect(size_t n) {
  if (kl_unlikely(n > 100)) {
    return malloc(n * n);
  } else {
    return malloc(n + 2);
  }
}






void fibonacci(KlState* state);
void concat(KlState* state);

int main(void) {
  KlMM klmm;
  klmm_init(&klmm, 1024);
  KlState* state = klapi_new_state(&klmm);
  fibonacci(state);
  //concat(state);
  size_t narg = 1;
  klapi_pushint(state, 35);
  //klapi_pushstring(state, "hello,");
  //klapi_pushstring(state, " ");
  //klapi_pushstring(state, "world!");
  clock_t t = clock();
  KlException exception = klapi_call(state, klapi_access(state, -1 - narg), narg, 1);
  printf("%f\n", (clock() - t) / (float)CLOCKS_PER_SEC);
  if (exception) {
    fprintf(stderr, "%s\n", state->throwinfo.exception.message);
    return 0;
  }
  //printf("%s", klstring_content(klapi_getstring(state, -1)));
  printf("fibonacci(%d) = %zd\n", 35, klapi_getint(state, -1));
  klmm_destroy(&klmm);
  return 0;
}

void fibonacci(KlState* state) {
  KlMM* klmm = klstate_getmm(state);
  KlInstruction* code = (KlInstruction*)klmm_alloc(klmm, 100 * sizeof (KlInstruction));
  KlKFunction* kfunc = klkfunc_alloc(klmm, code, 100, 1, 0, 100, 1);
  //KlRefInfo* refinfo = klkfunc_refinfo(kfunc);
  //refinfo[0].index = 0;
  //refinfo[0].in_stack = true;
  KlValue* constants = klkfunc_constants(kfunc);
  klapi_pushstring(state, "fibonacci");
  klvalue_setvalue(&constants[0], klapi_access(state, -1));
  code[0] = klinst_blei(0, 1, 8);
  code[1] = klinst_loadglobal(1, 0);
  code[2] = klinst_subi(2, 0, 1);
  code[3] = klinst_call(1, 1, 1);
  code[4] = klinst_loadglobal(2, 0);
  code[5] = klinst_subi(3, 0, 2);
  code[6] = klinst_call(2, 1, 1);
  code[7] = klinst_add(1, 1, 2);
  code[8] = klinst_jmp(1);
  code[9] = klinst_move(1, 0);
  code[10] = klinst_return1(1);
  //klapi_pushnil(state, 1);
  KlKClosure* kclo = klkclosure_create(klmm, kfunc, klapi_access(state, -1), &state->reflist, NULL);
  klkfunc_initdone(klmm, kfunc);
  klapi_setobj(state, -1, kclo, KL_KCLOSURE);
  klapi_storeglobal(state, klstrpool_new_string(state->strpool, "fibonacci"));
}

void concat(KlState* state) {
  KlMM* klmm = klstate_getmm(state);
  KlInstruction* code = (KlInstruction*)klmm_alloc(klmm, 100 * sizeof (KlInstruction));
  KlKFunction* kfunc = klkfunc_alloc(klmm, code, 100, 1, 0, 100, 0);
  KlValue* constants = klkfunc_constants(kfunc);
  klapi_pushstring(state, "");
  klvalue_setvalue(&constants[0], klapi_access(state, -1));
  code[0] = klinst_adjustargs();
  code[1] = klinst_loadc(0, 0);
  code[2] = klinst_vforprep(1, 4);
  code[3] = klinst_move(3, 0);
  code[4] = klinst_move(4, 2);
  code[5] = klinst_concat(0, 3, 2);
  code[6] = klinst_vforloop(1, -4);
  code[7] = klinst_return1(0);
  KlKClosure* kclo = klkclosure_create(klmm, kfunc, klapi_access(state, -1), &state->reflist, NULL);
  klkfunc_initdone(klmm, kfunc);
  klapi_setobj(state, -1, kclo, KL_KCLOSURE);
}
