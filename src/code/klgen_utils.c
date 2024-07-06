#include "include/code/klgen_utils.h"

void klgen_loadval(KlGenUnit* gen, KlCStkId target, KlCodeVal val, KlFilePosition position) {
  switch (val.kind) {
    case KLVAL_STACK: {
      if (target != val.index)
        klgen_emitmove(gen, target, val.index, 1, position);
      break;
    }
    case KLVAL_REF: {
      klgen_emit(gen, klinst_loadref(target, val.index), position);
      break;
    }
    case KLVAL_NIL: {
      klgen_emitloadnils(gen, target, 1, position);
      break;
    }
    case KLVAL_BOOL: {
      klgen_emit(gen, klinst_loadbool(target, val.boolval), position);
      break;
    }
    case KLVAL_STRING: {
      size_t conidx = klgen_newstring(gen, val.string);
      klgen_emit(gen, klinst_loadc(target, conidx), position);
      break;
    }
    case KLVAL_INTEGER: {
      KlInstruction inst;
      if (klinst_inrange(val.intval, 16)) {
        inst = klinst_loadi(target, val.intval);
      } else {
        size_t conidx = klgen_newinteger(gen, val.intval);
        inst = klinst_loadc(target, conidx);
      }
      klgen_emit(gen, inst, position);
      break;
    }
    case KLVAL_FLOAT: {
      size_t conidx = klgen_newfloat(gen, val.floatval);
      klgen_emit(gen, klinst_loadc(target, conidx), position);
      break;
    }
    default: {
      kl_assert(false, "control flow should not reach here");
      break;
    }
  }
}

