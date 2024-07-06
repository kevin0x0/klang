#ifndef _KLANG_INCLUDE_CODE_KLGEN_UTILS_H_
#define _KLANG_INCLUDE_CODE_KLGEN_UTILS_H_

#include "include/code/klgen.h"
#include "include/code/klgen_emit.h"

void klgen_loadval(KlGenUnit* gen, KlCStkId target, KlCodeVal val, KlFilePosition position);
static inline void klgen_putonstack(KlGenUnit* gen, KlCodeVal* val, KlFilePosition position);
static inline void klgen_putonstktop(KlGenUnit* gen, KlCodeVal* val, KlFilePosition position);


static inline void klgen_putonstack(KlGenUnit* gen, KlCodeVal* val, KlFilePosition position) {
  if (val->kind == KLVAL_STACK) return;
  KlCStkId stkid = klgen_stackalloc1(gen);
  klgen_loadval(gen, stkid, *val, position);
  val->kind = KLVAL_STACK;
  val->index = stkid;
}

static inline void klgen_putonstktop(KlGenUnit* gen, KlCodeVal* val, KlFilePosition position) {
  KlCStkId stkid = klgen_stacktop(gen);
  klgen_putonstack(gen, val, position);
  if (val->index != stkid)
    klgen_emit(gen, klinst_move(stkid, val->index), position);
  klgen_stackfree(gen, stkid + 1);
}

#endif
