#include "include/klapi.h"
#include "include/value/klclosure.h"
#include "include/value/klcoroutine.h"
#include "include/value/klstring.h"
#include "include/value/klvalue.h"
#include "include/lang/klinst.h"
#include <stdio.h>
#include <time.h>

void fibonacci(KlState* state);
void concat(KlState* state);
void arithsum(KlState* state);
void coroutine(KlState* state);
void gctest(KlState* state);

int main(void) {
  KlMM klmm;
  klmm_init(&klmm, 1024);
  KlState* state = klapi_new_state(&klmm);
  fibonacci(state);
  //concat(state);
  size_t narg = 1;
  klapi_pushint(state, 36);
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
  //printf("%s\n", klstring_content(klapi_getstring(state, -1)));
  //printf("\n%c", klstring_content(klapi_getstring(state, -1))[klstring_length(klapi_getstring(state, -1))]);
  printf("fibonacci(%d) = %lld\n", 35, klapi_getint(state, -1));
  klmm_destroy(&klmm);
  return 0;
}

void gctest(KlState* state) {
  KlMM* klmm = klstate_getmm(state);
  KlInstruction* code = (KlInstruction*)klmm_alloc(klmm, 100 * sizeof (KlInstruction));
  KlKFunction* kfunc = klkfunc_alloc(klmm, code, 100, 2, 0, 0, 100, 0);
  // KlRefInfo* refinfo = klkfunc_refinfo(kfunc);
  KlValue* constants = klkfunc_constants(kfunc);
  klapi_pushint(state, 100000000);
  klvalue_setvalue(&constants[0], klapi_access(state, -1));
  klapi_setstring(state, -1, "name");
  klvalue_setvalue(&constants[1], klapi_access(state, -1));
  code[0] = klinst_adjustargs();
  code[1] = klinst_loadi(0, 0);
  code[2] = klinst_loadc(1, 0);
  code[3] = klinst_loadnil(2, 0);
  code[4] = klinst_iforprep(0, 2);
  code[5] = klinst_loadc(3, 1);
  code[6] = klinst_iforloop(0, -2);
  code[7] = klinst_return1(0);
  // klapi_pushnil(state, 1);
  KlKClosure* kclo = klkclosure_create(klmm, kfunc, klapi_access(state, -1), &state->reflist, NULL);
  klkfunc_initdone(klmm, kfunc);
  klapi_setobj(state, -1, kclo, KL_KCLOSURE);
  // klreflist_close(&state->reflist, klstate_getval(state, -1), klstate_getmm(state));
  // klapi_storeglobal(state, klstrpool_new_string(state->strpool, "fibonacci"));
}

void fibonacci(KlState* state) {
  KlMM* klmm = klstate_getmm(state);
  KlInstruction* code = (KlInstruction*)klmm_alloc(klmm, 100 * sizeof (KlInstruction));
  KlKFunction* kfunc = klkfunc_alloc(klmm, code, 100, 0, 1, 0, 100, 1);
  KlRefInfo* refinfo = klkfunc_refinfo(kfunc);
  refinfo[0].index = 0;
  refinfo[0].on_stack = true;
  // KlValue* constants = klkfunc_constants(kfunc);
  // klapi_pushstring(state, "fibonacci");
  // klvalue_setvalue(&constants[0], klapi_access(state, -1));
  code[0] = klinst_lei(0, 1);
  code[1] = klinst_condjmp(false, 2);
  code[2] = klinst_testset(1, 0);
  code[3] = klinst_condjmp(true, 7);
  code[4] = klinst_loadref(1, 0);
  code[5] = klinst_subi(2, 0, 1);
  code[6] = klinst_scall(1, 1, 1);
  code[7] = klinst_loadref(2, 0);
  code[8] = klinst_subi(3, 0, 2);
  code[9] = klinst_scall(2, 1, 1);
  code[10] = klinst_add(1, 1, 2);
  code[11] = klinst_return1(1);
  klapi_pushnil(state, 1);
  KlKClosure* kclo = klkclosure_create(klmm, kfunc, klapi_access(state, -1), &state->reflist, NULL);
  klkfunc_initdone(klmm, kfunc);
  klapi_setobj(state, -1, kclo, KL_KCLOSURE);
  klreflist_close(&state->reflist, klstate_getval(state, -1), klstate_getmm(state));
  // klapi_storeglobal(state, klstrpool_new_string(state->strpool, "fibonacci"));
}

void coroutine(KlState* state) {
  fibonacci(state);
  KlState* co = klco_create(state, klvalue_getobj(klstate_getval(state, -1), KlKClosure*));
  klapi_setobj(state, -1, co, KL_COROUTINE);
}

void concat(KlState* state) {
  KlMM* klmm = klstate_getmm(state);
  KlInstruction* code = (KlInstruction*)klmm_alloc(klmm, 100 * sizeof (KlInstruction));
  KlKFunction* kfunc = klkfunc_alloc(klmm, code, 100, 1, 0, 0, 100, 0);
  KlValue* constants = klkfunc_constants(kfunc);
  klapi_pushstring(state, "");
  klvalue_setvalue(&constants[0], klapi_access(state, -1));
  code[0] = klinst_adjustargs();
  code[1] = klinst_loadc(0, 0);
  code[2] = klinst_loadi(1, 1);
  code[3] = klinst_vforprep(1, 4);
  code[4] = klinst_move(4, 0);
  code[5] = klinst_move(5, 3);
  code[6] = klinst_concat(0, 4, 2);
  code[7] = klinst_vforloop(1, -4);
  code[8] = klinst_return1(0);
  KlKClosure* kclo = klkclosure_create(klmm, kfunc, klapi_access(state, -1), &state->reflist, NULL);
  klkfunc_initdone(klmm, kfunc);
  klapi_setobj(state, -1, kclo, KL_KCLOSURE);
}

void arithsum(KlState* state) {
  KlMM* klmm = klstate_getmm(state);
  KlInstruction* code = (KlInstruction*)klmm_alloc(klmm, 100 * sizeof (KlInstruction));
  KlKFunction* kfunc = klkfunc_alloc(klmm, code, 100, 1, 0, 0, 100, 0);
  KlValue* constants = klkfunc_constants(kfunc);
  klapi_pushint(state, 1000000000);
  klvalue_setvalue(&constants[0], klapi_access(state, -1));
  code[0] = klinst_loadi(0, 0); /* sum */
  code[1] = klinst_loadi(1, 0); /* i */
  code[2] = klinst_loadi(2, 1); /* j */
  code[3] = klinst_gec(1, 0);
  code[4] = klinst_condjmp(true, 5);
  code[5] = klinst_mul(3, 1, 2);
  code[6] = klinst_add(0, 0, 3);
  code[7] = klinst_neg(2, 2);
  code[8] = klinst_addi(1, 1, 1);
  code[9] = klinst_jmp(-7);
  code[10] = klinst_return1(0);

  KlKClosure* kclo = klkclosure_create(klmm, kfunc, klapi_access(state, -1), &state->reflist, NULL);
  klkfunc_initdone(klmm, kfunc);
  klapi_setobj(state, -1, kclo, KL_KCLOSURE);
}
