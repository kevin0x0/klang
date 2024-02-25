#include "klang/include/klapi.h"
#include "klang/include/value/klclosure.h"
#include <stdio.h>

void newclosure(KlState* state);

int main(void) {
  KlMM klmm;
  klmm_init(&klmm, 1024);
  KlState* state = klapi_new_state(&klmm);
  newclosure(state);
  klapi_pushint(state, 35);

  KlException exception = klapi_call(state, klapi_access(state, -2), 1, 1);
  if (exception) {
    fprintf(stderr, "%s\n", state->throwinfo.exception.message);
    return 0;
  }
  printf("fibonacci(%d) = %zd\n", 35, klapi_getint(state, -1));
  klmm_destroy(&klmm);
  return 0;
}


void newclosure(KlState* state) {
  KlMM* klmm = klstate_getmm(state);
  KlInstruction* code = (KlInstruction*)klmm_alloc(klmm, 100 * sizeof (KlInstruction));
  KlKFunction* kfunc = klkfunc_alloc(klmm, code, 100, 0, 1, 100, 1);
  KlRefInfo* refinfo = klkfunc_refinfo(kfunc);
  refinfo[0].index = 0;
  refinfo[0].in_stack = true;
  code[0] = klinst_blei(0, 1, 8);
  code[1] = klinst_loadref(1, 0);
  code[2] = klinst_subi(2, 0, 1);
  code[3] = klinst_call(1, 1, 1);
  code[4] = klinst_loadref(2, 0);
  code[5] = klinst_subi(3, 0, 2);
  code[6] = klinst_call(2, 1, 1);
  code[7] = klinst_add(1, 1, 2);
  code[8] = klinst_jmp(1);
  code[9] = klinst_move(1, 0);
  code[10] = klinst_return1(1);
  klapi_pushnil(state, 1);
  KlKClosure* kclo = klkclosure_create(klmm, kfunc, klstate_getval(state, -1), &state->reflist, NULL);
  klkfunc_initdone(klmm, kfunc);
  klapi_setobj(state, -1, kclo, KL_KCLOSURE);
}
