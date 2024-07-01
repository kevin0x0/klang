#ifndef _KLANG_INCLUDE_CODE_KLGEN_EXPRBOOL_H_
#define _KLANG_INCLUDE_CODE_KLGEN_EXPRBOOL_H_

#include "include/code/klgen.h"
#include "include/code/klcodeval.h"
#include "include/ast/klast.h"
#include "include/ast/klast.h"


KlCodeVal klgen_exprbool(KlGenUnit* gen, KlAst* ast, bool jumpcond);
KlCodeVal klgen_exprboolval(KlGenUnit* gen, KlAst* ast, KlCStkId target);
void klgen_jumpto(KlGenUnit* gen, KlCodeVal jmplist, KlCPC jmppos);


static inline void klgen_setoffset(KlGenUnit* gen, KlInstruction* jmpinst, int offset) {
  if (KLINST_GET_OPCODE(*jmpinst) == KLOPCODE_JMP) {
    if (!klinst_inrange(offset, 24))
      klgen_error_fatal(gen, "jump too far, can not generate code");
    *jmpinst = klinst_jmp(offset);
  } else {
    if (!klinst_inrange(offset, 16))
      klgen_error_fatal(gen, "jump too far, can not generate code");
    KlOpcode opcode = KLINST_GET_OPCODE(*jmpinst);
    KlCIdx AorX = KLINST_AI_GETA(*jmpinst);
    *jmpinst = klinst_AI(opcode, AorX, offset);
  }
}

static inline KlCodeVal klgen_mergejmplist(KlGenUnit* gen, KlCodeVal jmplst1, KlCodeVal jmplst2) {
  kl_assert(jmplst1.kind == KLVAL_JMPLIST && jmplst2.kind == KLVAL_JMPLIST, "");
  KlInstruction* jlst2tail = klinstarr_access(&gen->code, jmplst2.jmplist.tail);
  klgen_setoffset(gen, jlst2tail, jmplst1.jmplist.head - jmplst2.jmplist.tail);
  jmplst2.jmplist.tail = jmplst1.jmplist.tail;
  return jmplst2;
}

static inline void klgen_mergejmplist_maynone(KlGenUnit* gen, KlCodeVal* jmplst1, KlCodeVal jmplst2) {
  *jmplst1 = jmplst1->kind == KLVAL_NONE ? jmplst2 : klgen_mergejmplist(gen, *jmplst1, jmplst2);
}
#endif
