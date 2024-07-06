#ifndef _KLANG_INCLUDE_CODE_KLGEN_EMIT_H_
#define _KLANG_INCLUDE_CODE_KLGEN_EMIT_H_

#include "include/code/klgen.h"


static inline KlCPC klgen_emit(KlGenUnit* gen, KlInstruction inst, KlFilePosition position);
void klgen_emitloadnils(KlGenUnit* gen, KlCStkId target, size_t nnil, KlFilePosition position);
void klgen_emitmove(KlGenUnit* gen, KlCStkId target, KlCStkId from, size_t nval, KlFilePosition position);
void klgen_emitmethod(KlGenUnit* gen, KlCStkId obj, KlCStkId method, size_t narg, size_t nret, KlCStkId retpos, KlFilePosition position);
void klgen_emitcall(KlGenUnit* gen, KlCStkId callable, size_t narg, size_t nret, KlCStkId retpos, KlFilePosition position);


static inline KlCPC klgen_emit(KlGenUnit* gen, KlInstruction inst, KlFilePosition position) {
  KlCPC pc = klgen_currentpc(gen);
  if (kl_unlikely(!klinstarr_push_back(&gen->code, inst)))
    klgen_error_fatal(gen, "out of memory");
  if (gen->config->posinfo)
    klfparr_push_back(&gen->position, position);
  klgen_unmarkjmptarget(gen);
  return pc;
}

#endif
