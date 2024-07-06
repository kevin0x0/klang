#include "include/code/klgen_emit.h"


void klgen_emitloadnils(KlGenUnit* gen, KlCStkId target, size_t nnil, KlFilePosition position) {
  if (klgen_currentpc(gen) == 0) {
    klgen_emit(gen, klinst_loadnil(target, nnil), position);
    return;
  }
  KlInstruction* previnst = klinstarr_back(&gen->code);
  if (KLINST_GET_OPCODE(*previnst) == KLOPCODE_LOADNIL && /* same instruction */
      !gen->jmpinfo.isjmptarget) {                           /* current block is not entry point of a basic block */
    KlCStkId prev_target = KLINST_AX_GETA(*previnst);
    size_t prev_nnil = KLINST_AX_GETX(*previnst);
    if ((prev_target <= target && target <= prev_target + prev_nnil) ||
        (target <= prev_target && prev_target <= target + nnil)) {
      KlCStkId new_target = prev_target < target ? prev_target : target;
      size_t new_nnil = prev_target + prev_nnil > target + nnil
                      ? (size_t)(prev_target + prev_nnil - new_target)
                      : target + nnil - new_target;
      *previnst = klinst_loadnil(new_target, new_nnil);
      return;
    } /* else fall through */
  }
  klgen_emit(gen, klinst_loadnil(target, nnil), position);
}

void klgen_emitmove(KlGenUnit* gen, KlCStkId target, KlCStkId from, size_t nmove, KlFilePosition position) {
  if (nmove == 0) return;
  kl_assert(from != target, "");

  if (nmove == KLINST_VARRES) {
    klgen_emit(gen, klinst_multimove(target, from, nmove), position);
    return;
  }

  if (klgen_currentpc(gen) == 0) {
    klgen_emit(gen, nmove == 1 ? klinst_move(target, from) : klinst_multimove(target, from, nmove), position);
    return;
  }

  KlInstruction* previnst = klinstarr_back(&gen->code);
  KlOpcode prev_opcode = KLINST_GET_OPCODE(*previnst);
  if (gen->jmpinfo.isjmptarget ||  /* is entry point of current basic block? */
      (prev_opcode != KLOPCODE_MOVE && prev_opcode != KLOPCODE_MULTIMOVE)) {
    klgen_emit(gen, nmove == 1 ? klinst_move(target, from) : klinst_multimove(target, from, nmove), position);
    return;
  }

  KlCStkId prev_target = prev_opcode == KLOPCODE_MOVE ? KLINST_ABC_GETA(*previnst) : KLINST_ABX_GETA(*previnst);
  KlCStkId prev_from = prev_opcode == KLOPCODE_MOVE ? KLINST_ABC_GETB(*previnst) : KLINST_ABX_GETB(*previnst);
  size_t prev_nmove = prev_opcode == KLOPCODE_MOVE ? 1 : KLINST_ABX_GETX(*previnst);
  if (prev_from + target == from + prev_target &&                     /* same move length */
      ((from <= prev_from && prev_from <= from + nmove) ||
       (prev_from <= from && from <= prev_from + prev_nmove)) &&       /* continuous move */
      (prev_target >= from + nmove || prev_target + nmove <= from)) { /* previous move does not modify source data of this move */
    KlCStkId new_target = prev_target < target ? prev_target : target;
    KlCStkId new_from = prev_from < from ? prev_from : from;
    size_t new_nmove = prev_target + prev_nmove > target + nmove
                     ? prev_target + prev_nmove - new_target
                     : target + nmove - new_target;
    *previnst = new_nmove == 1 ? klinst_move(new_target, new_from) : klinst_multimove(new_target, new_from, new_nmove);
    return;
  } /* else fall through */
  klgen_emit(gen, nmove == 1 ? klinst_move(target, from) : klinst_multimove(target, from, nmove), position);
}


void klgen_emitmethod(KlGenUnit* gen, KlCStkId obj, KlCStkId method, size_t narg, size_t nret, KlCStkId retpos, KlFilePosition position) {
  klgen_emit(gen, klinst_method(obj, method), position);
  klgen_emit(gen, klinst_methodextra(narg, nret, retpos), position);
}

void klgen_emitcall(KlGenUnit* gen, KlCStkId callable, size_t narg, size_t nret, KlCStkId retpos, KlFilePosition position) {
  klgen_emit(gen, klinst_call(callable), position);
  klgen_emit(gen, klinst_callextra(narg, nret, retpos), position);
}
