#ifndef KEVCC_KLANG_INCLUDE_CODE_KLGEN_EXPRBOOL_H
#define KEVCC_KLANG_INCLUDE_CODE_KLGEN_EXPRBOOL_H

#include "include/code/klgen.h"
#include "include/code/klcodeval.h"
#include "include/cst/klcst.h"
#include "include/cst/klcst_expr.h"


KlCodeVal klgen_exprbool(KlGenUnit* gen, KlCst* cst, bool jumpcond);
KlCodeVal klgen_exprnot(KlGenUnit* gen, KlCstPre* notcst, bool jumpcond);
KlCodeVal klgen_expror(KlGenUnit* gen, KlCstBin* orcst, bool jumpcond);
KlCodeVal klgen_exprand(KlGenUnit* gen, KlCstBin* andcst, bool jumpcond);

KlCodeVal klgen_exprboolval(KlGenUnit* gen, KlCst* cst, size_t target);
void klgen_setinstjmppos(KlGenUnit* gen, KlCodeVal jmplist, size_t jmppos);


static inline void klgen_setoffset(KlGenUnit* gen, KlInstruction* jmpinst, int offset) {
  if (KLINST_GET_OPCODE(*jmpinst) == KLOPCODE_JMP) {
    if (!klinst_inrange(offset, 24))
      klgen_error_fatal(gen, "jump too far, can not generate code");
    *jmpinst = klinst_jmp(offset);
  } else {
    if (!klinst_inrange(offset, 16))
      klgen_error_fatal(gen, "jump too far, can not generate code");
    uint8_t opcode = KLINST_GET_OPCODE(*jmpinst);
    uint8_t AorX = KLINST_AI_GETA(*jmpinst);
    *jmpinst = klinst_AI(opcode, AorX, offset);
  }
}

static inline KlCodeVal klgen_mergejmp(KlGenUnit* gen, KlCodeVal jmplst1, KlCodeVal jmplst2) {
  kl_assert(jmplst1.kind == KLVAL_JMP && jmplst2.kind == KLVAL_JMP, "");
  KlInstruction* jlst2tail = klinstarr_access(&gen->code, jmplst2.jmplist.tail);
  klgen_setoffset(gen, jlst2tail, jmplst1.jmplist.head - jmplst2.jmplist.tail);
  jmplst2.jmplist.tail = jmplst1.jmplist.tail;
  return jmplst2;
}

static inline void klgen_mergejmp_maynone(KlGenUnit* gen, KlCodeVal* jmplst1, KlCodeVal jmplst2) {
  *jmplst1 = jmplst1->kind == KLVAL_NONE ? jmplst2 : klgen_mergejmp(gen, *jmplst1, jmplst2);
}
#endif
