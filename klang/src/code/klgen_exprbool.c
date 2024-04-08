#include "klang/include/code/klgen_exprbool.h"
#include "klang/include/code/klcode.h"
#include "klang/include/code/klcodeval.h"
#include "klang/include/code/klcontbl.h"
#include "klang/include/code/klgen.h"
#include "klang/include/code/klgen_expr.h"
#include "klang/include/value/klvalue.h"
#include "klang/include/vm/klinst.h"
#include <string.h>


static KlCodeVal klgen_exprrelrightnonstk(KlGenUnit* gen, KlCstBin* bincst, size_t oristktop, KlCodeVal left, KlCodeVal right, bool jumpcond);
static KlCodeVal klgen_exprboolset(KlGenUnit* gen, KlCst* boolcst, size_t target, bool setcond);

static inline int klgen_getoffset(KlInstruction jmpinst) {
  if (KLINST_GET_OPCODE(jmpinst) == KLOPCODE_JMP) {
    return KLINST_I_GETI(jmpinst);
  } else {
    return KLINST_AI_GETI(jmpinst);
  }
}

void klgen_setinstjmppos(KlGenUnit* gen, KlCodeVal jmplist, size_t jmppos) {
  if (jmplist.kind == KLVAL_NONE) return;
  KlInstruction* pc = klinstarr_access(&gen->code, jmplist.jmplist.head);
  KlInstruction* end = klinstarr_access(&gen->code, jmplist.jmplist.tail);
  KlInstruction* pjmppos = klinstarr_access(&gen->code, jmppos);
  while (pc != end) {
    int nextoffset = klgen_getoffset(*pc);
    klgen_setoffset(gen, pc, pjmppos - pc - 1);
    pc += nextoffset;
  }
  klgen_setoffset(gen, pc, pjmppos - pc - 1);
}

static KlCodeVal klgen_pushrelinst(KlGenUnit* gen, KlCstBin* relcst, size_t leftid, size_t rightid, bool jumpcond) {
  switch (relcst->op) {
    case KLTK_LT: {
      klgen_pushinst(gen, klinst_lt(leftid, rightid), klgen_cstposition(relcst));
      break;
    }
    case KLTK_LE: {
      klgen_pushinst(gen, klinst_le(leftid, rightid), klgen_cstposition(relcst));
      break;
    }
    case KLTK_GT: {
      klgen_pushinst(gen, klinst_gt(leftid, rightid), klgen_cstposition(relcst));
      break;
    }
    case KLTK_GE: {
      klgen_pushinst(gen, klinst_ge(leftid, rightid), klgen_cstposition(relcst));
      break;
    }
    case KLTK_EQ: {
      klgen_pushinst(gen, klinst_eq(leftid, rightid), klgen_cstposition(relcst));
      break;
    }
    case KLTK_IS: {
      klgen_pushinst(gen, klinst_is(leftid, rightid), klgen_cstposition(relcst));
      break;
    }
    case KLTK_NE: {
      klgen_pushinst(gen, klinst_ne(leftid, rightid), klgen_cstposition(relcst));
      break;
    }
    case KLTK_ISNOT: {
      klgen_pushinst(gen, klinst_is(leftid, rightid), klgen_cstposition(relcst));
      jumpcond = !jumpcond;
      break;
    }
    default: {
      kl_assert(false, "control flow should not reach here");
      break;
    }
  }
  size_t pc = klgen_pushinst(gen, klinst_condjmp(jumpcond, 0), klgen_cstposition(relcst));
  return klcodeval_jmp(pc);
}

static KlCodeVal klgen_pushrelinsti(KlGenUnit* gen, KlCstBin* relcst, size_t leftid, KlInt imm, bool jumpcond) {
  switch (relcst->op) {
    case KLTK_LT: {
      klgen_pushinst(gen, klinst_lti(leftid, imm), klgen_cstposition(relcst));
      break;
    }
    case KLTK_LE: {
      klgen_pushinst(gen, klinst_lei(leftid, imm), klgen_cstposition(relcst));
      break;
    }
    case KLTK_GT: {
      klgen_pushinst(gen, klinst_gti(leftid, imm), klgen_cstposition(relcst));
      break;
    }
    case KLTK_GE: {
      klgen_pushinst(gen, klinst_gei(leftid, imm), klgen_cstposition(relcst));
      break;
    }
    case KLTK_EQ: {
      if (jumpcond) {
        klgen_pushinst(gen, klinst_eqi(leftid, imm), klgen_cstposition(relcst));
      } else {
        klgen_pushinst(gen, klinst_nei(leftid, imm), klgen_cstposition(relcst));
      }
      size_t pc = klgen_pushinst(gen, klinst_condjmp(true, 0), klgen_cstposition(relcst));
      return klcodeval_jmp(pc);
    }
    case KLTK_NE: {
      if (jumpcond) {
        klgen_pushinst(gen, klinst_nei(leftid, imm), klgen_cstposition(relcst));
      } else {
        klgen_pushinst(gen, klinst_eqi(leftid, imm), klgen_cstposition(relcst));
      }
      size_t pc = klgen_pushinst(gen, klinst_condjmp(true, 0), klgen_cstposition(relcst));
      return klcodeval_jmp(pc);
    }
    default: {
      kl_assert(false, "control flow should not reach here");
      break;
    }
  }
  size_t pc = klgen_pushinst(gen, klinst_condjmp(jumpcond, 0), klgen_cstposition(relcst));
  return klcodeval_jmp(pc);
}

static KlCodeVal klgen_pushrelinstc(KlGenUnit* gen, KlCstBin* relcst, size_t leftid, size_t conidx, bool jumpcond) {
  switch (relcst->op) {
    case KLTK_LT: {
      klgen_pushinst(gen, klinst_ltc(leftid, conidx), klgen_cstposition(relcst));
      break;
    }
    case KLTK_LE: {
      klgen_pushinst(gen, klinst_lec(leftid, conidx), klgen_cstposition(relcst));
      break;
    }
    case KLTK_GT: {
      klgen_pushinst(gen, klinst_gtc(leftid, conidx), klgen_cstposition(relcst));
      break;
    }
    case KLTK_GE: {
      klgen_pushinst(gen, klinst_gec(leftid, conidx), klgen_cstposition(relcst));
      break;
    }
    case KLTK_EQ: {
      if (jumpcond) {
        klgen_pushinst(gen, klinst_eqc(leftid, conidx), klgen_cstposition(relcst));
      } else {
        klgen_pushinst(gen, klinst_nec(leftid, conidx), klgen_cstposition(relcst));
      }
      size_t pc = klgen_pushinst(gen, klinst_condjmp(true, 0), klgen_cstposition(relcst));
      return klcodeval_jmp(pc);
    }
    case KLTK_NE: {
      if (jumpcond) {
        klgen_pushinst(gen, klinst_nec(leftid, conidx), klgen_cstposition(relcst));
      } else {
        klgen_pushinst(gen, klinst_eqc(leftid, conidx), klgen_cstposition(relcst));
      }
      size_t pc = klgen_pushinst(gen, klinst_condjmp(true, 0), klgen_cstposition(relcst));
      return klcodeval_jmp(pc);
    }
    default: {
      kl_assert(false, "control flow should not reach here");
      break;
    }
  }
  size_t pc = klgen_pushinst(gen, klinst_condjmp(jumpcond, 0), klgen_cstposition(relcst));
  return klcodeval_jmp(pc);
}

static KlCodeVal klgen_relcomptime(KlGenUnit* gen, KlCstBin* bincst, KlCodeVal left, KlCodeVal right) {
  if (left.kind == KLVAL_INTEGER && right.kind == KLVAL_INTEGER) {
    KlBool cond;
    switch (bincst->op) {
      case KLTK_LT: cond = left.intval < right.intval; break;
      case KLTK_LE: cond = left.intval <= right.intval; break;
      case KLTK_GT: cond = left.intval > right.intval; break;
      case KLTK_GE: cond = left.intval >= right.intval; break;
      case KLTK_ISNOT:
      case KLTK_NE: cond = left.intval != right.intval; break;
      case KLTK_EQ:
      case KLTK_IS: cond = left.intval == right.intval; break;
      default: {
        kl_assert(false, "control flow should not reach here");
        return klcodeval_nil();
      }
    }
    return klcodeval_bool(cond);
  } else if (left.kind == KLVAL_STRING && right.kind == KLVAL_STRING) {
    int cmpres;
    if (left.string.length == right.string.length) {
      cmpres = strncmp(klstrtab_getstring(gen->strtab, left.string.id),
                       klstrtab_getstring(gen->strtab, right.string.id),
                       left.string.length);
    } else {
      cmpres = left.string.length > right.string.length ? 1 : -1;
    }
    KlBool cond;
    switch (bincst->op) {
      case KLTK_LT: cond = cmpres < 0; break;
      case KLTK_LE: cond = cmpres <= 0; break;
      case KLTK_GT: cond = cmpres > 0; break;
      case KLTK_GE: cond = cmpres >= 0; break;
      case KLTK_ISNOT:
      case KLTK_NE: cond = cmpres != 0; break;
      case KLTK_EQ:
      case KLTK_IS: cond = cmpres == 0; break;
      default: {
        kl_assert(false, "control flow should not reach here");
        return klcodeval_nil();
      }
    }
    return klcodeval_bool(cond);
  } else {
    KlFloat l = left.kind == KLVAL_INTEGER ? (KlFloat)left.intval : left.floatval;
    KlFloat r = right.kind == KLVAL_INTEGER ? (KlFloat)right.intval : right.floatval;
    KlBool cond;
    switch (bincst->op) {
      case KLTK_LT: cond = l < r; break;
      case KLTK_LE: cond = l <= r; break;
      case KLTK_GT: cond = l > r; break;
      case KLTK_GE: cond = l >= r; break;
      case KLTK_ISNOT:
      case KLTK_NE: cond = left.kind != right.kind || l != r; break;
      case KLTK_EQ:
      case KLTK_IS: cond = left.kind == right.kind && l == r; break;
      default: {
        kl_assert(false, "control flow should not reach here");
        return klcodeval_nil();
      }
    }
    return klcodeval_bool(cond);
  }
}

static KlCodeVal klgen_equalitycomptime(KlGenUnit* gen, KlCstBin* bincst, KlCodeVal left, KlCodeVal right) {
  kl_assert(klcodeval_isconstant(left) && klcodeval_isconstant(right), "");
  if (left.kind != right.kind) {
    return klcodeval_bool(bincst->op == KLTK_NE);
  } else {
    switch (left.kind) {
      case KLVAL_STRING:
      case KLVAL_FLOAT:
      case KLVAL_INTEGER: return klgen_relcomptime(gen, bincst, left, right);
      case KLVAL_NIL: return klcodeval_bool(bincst->op == KLTK_EQ || bincst->op == KLTK_IS);
      case KLVAL_BOOL: return klcodeval_bool(bincst->op == KLTK_EQ || bincst->op == KLTK_IS ? left.boolval == right.boolval : left.boolval != right.boolval);
      default: {
        kl_assert(false, "control flow should not reach here");
        return klcodeval_nil();
      }
    }
  }
}

static KlCodeVal klgen_exprrelleftliteral(KlGenUnit* gen, KlCstBin* bincst, KlCodeVal left, bool jumpcond) {
  /* left is not on the stack, so the stack top is not changed */
  size_t stktop = klgen_stacktop(gen);
  size_t currcodesize = klgen_currcodesize(gen);
  /* we put left on the stack first */
  KlCodeVal leftonstack = left;
  klgen_putinstack(gen, &leftonstack, klgen_cstposition(bincst->loperand));
  size_t stksize_before_evaluete_right = klgen_currcodesize(gen);
  KlCodeVal right = klgen_expr(gen, bincst->roperand);
  if ((klcodeval_isnumber(right) && klcodeval_isnumber(left)) ||
      (right.kind == KLVAL_STRING && right.kind == KLVAL_STRING)) {
    if (stksize_before_evaluete_right == klgen_currcodesize(gen))
      klgen_popinstto(gen, currcodesize);   /* pop the instruction that put left on stack */
    klgen_stackfree(gen, stktop);
    /* do compile time comparison */
    return klgen_relcomptime(gen, bincst, left, right);
  } else if (bincst->op == KLTK_NE || bincst->op == KLTK_EQ ||
             bincst->op == KLTK_IS || bincst->op == KLTK_ISNOT) {
    if (klcodeval_isconstant(right)) {
      if (stksize_before_evaluete_right == klgen_currcodesize(gen))
        klgen_popinstto(gen, currcodesize);   /* pop the instruction that put left on stack */
      klgen_stackfree(gen, stktop);
      return klgen_equalitycomptime(gen, bincst, left, right);
    }
  } /* else can not apply compile time comparison */
  if (right.kind == KLVAL_STACK) {
    /* now we are sure that left should indeed be put on the stack */
    klgen_stackfree(gen, stktop);
    return klgen_pushrelinst(gen, bincst, leftonstack.index, right.index, jumpcond);
  } else {
    return klgen_exprrelrightnonstk(gen, bincst, stktop, leftonstack, right, jumpcond);
  }
}

static KlCodeVal klgen_exprrelrightnonstk(KlGenUnit* gen, KlCstBin* bincst, size_t oristktop, KlCodeVal left, KlCodeVal right, bool jumpcond) {
  /* left must be on stack */
  kl_assert(left.kind == KLVAL_STACK, "");
  if (bincst->op == KLTK_IS || bincst->op == KLTK_ISNOT) {
    klgen_putinstack(gen, &right, klgen_cstposition(bincst->roperand));
    klgen_stackfree(gen, oristktop);
    return klgen_pushrelinst(gen, bincst, left.index, right.index, jumpcond);
  }
  switch (right.kind) {
    case KLVAL_INTEGER: {
      KlCodeVal res;
      if (klinst_inrange(right.intval, 8)) {
        res = klgen_pushrelinsti(gen, bincst, left.index, right.intval, jumpcond);
      } else {
        KlConstant con = { .type = KL_INT, .intval = right.intval };
        KlConEntry* conent = klcontbl_search(gen->contbl, &con);
        if (!conent) {
          size_t nextconidx = klcontbl_nextindex(gen->contbl);
          if (!klinst_inurange(nextconidx, 8) && klinst_inrange(right.intval, 16)) {
            size_t stktop = klgen_stackalloc1(gen);
            klgen_pushinst(gen, klinst_loadi(stktop, right.intval), klgen_cstposition(bincst->roperand));
            klgen_stackfree(gen, oristktop);
            return klgen_pushrelinst(gen, bincst, left.index, stktop, jumpcond);
          }
          conent = klcontbl_insert(gen->contbl, &con);
          klgen_oomifnull(gen, conent);
        }
        if (klinst_inurange(conent->index, 8)) {
          res = klgen_pushrelinstc(gen, bincst, left.index, conent->index, jumpcond);
        } else {
          size_t stktop = klgen_stackalloc1(gen);
          klgen_pushinst(gen, klinst_loadc(stktop, conent->index), klgen_cstposition(bincst->roperand));
          res = klgen_pushrelinst(gen, bincst, left.index, stktop, jumpcond);
        }
      }
      klgen_stackfree(gen, oristktop);
      return res;
    }
    case KLVAL_FLOAT: {
      KlConstant con = { .type = KL_FLOAT, .intval = right.floatval };
      KlConEntry* conent = klcontbl_get(gen->contbl, &con);
      klgen_oomifnull(gen, conent);
      KlCodeVal res;
      if (klinst_inurange(conent->index, 8)) {
        res = klgen_pushrelinstc(gen, bincst, left.index, conent->index, jumpcond);
      } else {
        size_t stktop = klgen_stackalloc1(gen);
        klgen_pushinst(gen, klinst_loadc(stktop, conent->index), klgen_cstposition(bincst->roperand));
        res = klgen_pushrelinst(gen, bincst, left.index, stktop, jumpcond);
      }
      klgen_stackfree(gen, oristktop);
      return res;
    }
    case KLVAL_STRING: {
      KlConstant con = { .type = KL_STRING, .string = right.string };
      KlConEntry* conent = klcontbl_get(gen->contbl, &con);
      klgen_oomifnull(gen, conent);
      KlCodeVal res;
      if (klinst_inurange(conent->index, 8)) {
        res = klgen_pushrelinstc(gen, bincst, left.index, conent->index, jumpcond);
      } else {
        size_t stktop = klgen_stackalloc1(gen);
        klgen_pushinst(gen, klinst_loadc(stktop, conent->index), klgen_cstposition(bincst->roperand));
        res = klgen_pushrelinst(gen, bincst, left.index, stktop, jumpcond);
      }
      klgen_stackfree(gen, oristktop);
      return res;
    }
    case KLVAL_REF:
    case KLVAL_BOOL:
    case KLVAL_NIL: {
      klgen_putinstack(gen, &right, klgen_cstposition(bincst->roperand));
      klgen_stackfree(gen, oristktop);
      return klgen_pushrelinst(gen, bincst, left.index, right.index, jumpcond);
    }
    default: {
      kl_assert(false, "control flow should not reach here");
      return klcodeval_nil();
    }
  }
}

KlCodeVal klgen_exprrelation(KlGenUnit* gen, KlCstBin* relcst, bool jumpcond) {
  size_t oristktop = klgen_stacktop(gen);
  KlCodeVal left = klgen_expr(gen, relcst->loperand);
  if (klcodeval_isconstant(left)) {
    return klgen_exprrelleftliteral(gen, relcst, left, jumpcond);
  } else {
    klgen_putinstack(gen, &left, klgen_cstposition(relcst->loperand));
  }
  /* now left is on stack */
  KlCodeVal right = klgen_expr(gen, relcst->roperand);
  if (left.kind != KLVAL_STACK)
    return klgen_exprrelrightnonstk(gen, relcst, oristktop, left, right, jumpcond);
  /* now both are on stack */
  klgen_stackfree(gen, oristktop);
  return klgen_pushrelinst(gen, relcst, left.index, right.index, jumpcond);
}

KlCodeVal klgen_exprbool(KlGenUnit* gen, KlCst* cst, bool jumpcond) {
  if (klcst_kind(cst) == KLCST_EXPR_PRE && klcast(KlCstPre*, cst)->op == KLTK_NOT) {
    return klgen_exprnot(gen, klcast(KlCstPre*, cst), jumpcond);
  } else if (klcst_kind(cst) == KLCST_EXPR_BIN) {
    KlCstBin* bincst = klcast(KlCstBin*, cst);
    if (bincst->op == KLTK_AND)
      return klgen_exprand(gen, bincst, jumpcond);
    if (bincst->op == KLTK_OR)
      return klgen_expror(gen, bincst, jumpcond);
    if (kltoken_isrelation(bincst->op))
      return klgen_exprrelation(gen, bincst, jumpcond);
    /* else is other binary expression, fallthrough */
  } else if (klcst_kind(cst) == KLCST_EXPR_TUPLE) {
    KlCstTuple* tuple = klcast(KlCstTuple*, cst);
    KlCst* lastelem = tuple->nelem == 0 ? NULL : tuple->elems[tuple->nelem - 1];
    if (lastelem && klcst_isboolexpr(lastelem = klgen_exprpromotion(lastelem))) {
      klgen_tuple_evaluate(gen, tuple, tuple->nelem - 1);
      return klgen_exprbool(gen, lastelem, jumpcond);
    }
    /* else the tuple should be evaluated by klgen_expr, fallthrough */
  }
  KlCodeVal res = klgen_expr(gen, cst);
  if (klcodeval_isconstant(res)) return res;
  size_t stktop = klgen_stacktop(gen);
  klgen_putinstack(gen, &res, klgen_cstposition(cst));
  size_t pc = klgen_pushinst(gen, jumpcond ? klinst_truejmp(res.index, 0) : klinst_falsejmp(res.index, 0), klgen_cstposition(cst));
  klgen_stackfree(gen, stktop);
  return klcodeval_jmp(pc);
}

KlCodeVal klgen_exprnot(KlGenUnit* gen, KlCstPre* notcst, bool jumpcond) {
  KlCodeVal jmp = klgen_exprbool(gen, notcst->operand, !jumpcond);
  if (klcodeval_isconstant(jmp))
    return klcodeval_bool(klcodeval_isfalse(jmp));
  return jmp;
}

KlCodeVal klgen_expror(KlGenUnit* gen, KlCstBin* orcst, bool jumpcond) {
  KlCodeVal ljmp = klgen_exprbool(gen, orcst->loperand, true);
  if (klcodeval_isconstant(ljmp)) {
    if (klcodeval_istrue(ljmp)) return ljmp;
    return klgen_exprbool(gen, orcst->roperand, jumpcond);
  }
  KlCodeVal rjmp = klgen_exprbool(gen, orcst->roperand, jumpcond);
  if (klcodeval_isconstant(rjmp)) {
    size_t stktop = klgen_stacktop(gen);
    klgen_putinstack(gen, &rjmp, klgen_cstposition(orcst->roperand));
    rjmp = klcodeval_jmp(klgen_pushinst(gen,
                                        jumpcond ? klinst_truejmp(rjmp.index, 0)
                                                 : klinst_falsejmp(rjmp.index, 0),
                                        klgen_cstposition(orcst->roperand)));
    klgen_stackfree(gen, stktop);
  }
  if (jumpcond) {
    return klgen_mergejmp(gen, ljmp, rjmp);
  } else {
    klgen_setinstjmppos(gen, ljmp, klgen_currcodesize(gen));
    return rjmp;
  }
}

KlCodeVal klgen_exprand(KlGenUnit* gen, KlCstBin* andcst, bool jumpcond) {
  KlCodeVal ljmp = klgen_exprbool(gen, andcst->loperand, false);
  if (klcodeval_isconstant(ljmp)) {
    if (klcodeval_isfalse(ljmp)) return ljmp;
    return klgen_exprbool(gen, andcst->roperand, jumpcond);
  }
  KlCodeVal rjmp = klgen_exprbool(gen, andcst->roperand, jumpcond);
  if (klcodeval_isconstant(rjmp)) {
    size_t stktop = klgen_stacktop(gen);
    klgen_putinstack(gen, &rjmp, klgen_cstposition(andcst->roperand));
    rjmp = klcodeval_jmp(klgen_pushinst(gen,
                                        jumpcond ? klinst_truejmp(rjmp.index, 0)
                                                 : klinst_falsejmp(rjmp.index, 0),
                                        klgen_cstposition(andcst->roperand)));
    klgen_stackfree(gen, stktop);
  }
  if (jumpcond) {
    klgen_setinstjmppos(gen, ljmp, klgen_currcodesize(gen));
    return rjmp;
  } else {
    return klgen_mergejmp(gen, ljmp, rjmp);
  }
}

static KlCodeVal klgen_exprorset(KlGenUnit* gen, KlCstBin* orcst, size_t target, bool setcond) {
  if (setcond) {
    KlCodeVal lval = klgen_exprboolset(gen, orcst->loperand, target, true);
    if (klcodeval_isconstant(lval)) {
      if (klcodeval_istrue(lval)) return lval;
      return klgen_exprboolset(gen, orcst->roperand, target, setcond);
    }
    KlCodeVal rval = klgen_exprboolset(gen, orcst->roperand, target, setcond);
    if (klcodeval_isconstant(rval)) {
      size_t stktop = klgen_stacktop(gen);
      klgen_putinstack(gen, &rval, klgen_cstposition(orcst->roperand));
      klgen_pushinst(gen, klinst_testset(target, rval.index), klgen_cstposition(orcst->roperand));
      size_t pc = klgen_pushinst(gen, klinst_condjmp(setcond, 0), klgen_cstposition(orcst->roperand));
      klgen_mergejmp_maynone(gen, &gen->info.jumpinfo->truelist, klcodeval_jmp(pc));
      klgen_stackfree(gen, stktop);
    }
    return klcodeval_none();
  } else {
    KlCodeVal ljmp = klgen_exprbool(gen, orcst->loperand, true);
    if (klcodeval_isconstant(ljmp)) {
      if (klcodeval_istrue(ljmp)) return ljmp;
      return klgen_exprboolset(gen, orcst->roperand, target, setcond);
    }
    KlCodeVal rval = klgen_exprboolset(gen, orcst->roperand, target, setcond);
    if (klcodeval_isconstant(rval)) {
      size_t stktop = klgen_stacktop(gen);
      klgen_putinstack(gen, &rval, klgen_cstposition(orcst->roperand));
      klgen_pushinst(gen, klinst_testset(target, rval.index), klgen_cstposition(orcst->roperand));
      size_t pc = klgen_pushinst(gen, klinst_condjmp(setcond, 0), klgen_cstposition(orcst->roperand));
      klgen_mergejmp_maynone(gen, &gen->info.jumpinfo->falselist, klcodeval_jmp(pc));
      klgen_stackfree(gen, stktop);
    }
    klgen_setinstjmppos(gen, ljmp, klgen_currcodesize(gen));
    return klcodeval_none();
  }
}

static KlCodeVal klgen_exprandset(KlGenUnit* gen, KlCstBin* andcst, size_t target, bool setcond) {
  if (setcond) {
    KlCodeVal ljmp = klgen_exprbool(gen, andcst->loperand, false);
    if (klcodeval_isconstant(ljmp)) {
      if (klcodeval_isfalse(ljmp)) return ljmp;
      return klgen_exprboolset(gen, andcst->roperand, target, setcond);
    }
    KlCodeVal rval = klgen_exprboolset(gen, andcst->roperand, target, setcond);
    if (klcodeval_isconstant(rval)) {
      size_t stktop = klgen_stacktop(gen);
      klgen_putinstack(gen, &rval, klgen_cstposition(andcst->roperand));
      klgen_pushinst(gen, klinst_testset(target, rval.index), klgen_cstposition(andcst->roperand));
      size_t pc = klgen_pushinst(gen, klinst_condjmp(setcond, 0), klgen_cstposition(andcst->roperand));
      klgen_mergejmp_maynone(gen, &gen->info.jumpinfo->truelist, klcodeval_jmp(pc));
      klgen_stackfree(gen, stktop);
    }
    klgen_setinstjmppos(gen, ljmp, klgen_currcodesize(gen));
    return klcodeval_none();
  } else {
    KlCodeVal lval = klgen_exprboolset(gen, andcst->loperand, target, false);
    if (klcodeval_isconstant(lval)) {
      if (klcodeval_isfalse(lval)) return lval;
      return klgen_exprboolset(gen, andcst->roperand, target, setcond);
    }
    KlCodeVal rval = klgen_exprboolset(gen, andcst->roperand, target, setcond);
    if (klcodeval_isconstant(rval)) {
      size_t stktop = klgen_stacktop(gen);
      klgen_putinstack(gen, &rval, klgen_cstposition(andcst->roperand));
      klgen_pushinst(gen, klinst_testset(target, rval.index), klgen_cstposition(andcst->roperand));
      size_t pc = klgen_pushinst(gen, klinst_condjmp(setcond, 0), klgen_cstposition(andcst->roperand));
      klgen_mergejmp_maynone(gen, &gen->info.jumpinfo->falselist, klcodeval_jmp(pc));
      klgen_stackfree(gen, stktop);
    }
    return klcodeval_none();
  }
}

static KlCodeVal klgen_exprboolset_handleresult(KlGenUnit* gen, KlCodeVal res, bool setcond) {
  if (klcodeval_isconstant(res)) return res;
  KlCodeVal* jmplist = setcond ? &gen->info.jumpinfo->truelist : &gen->info.jumpinfo->falselist;
  klgen_mergejmp_maynone(gen, jmplist, res);
  return klcodeval_none();
}

static KlCodeVal klgen_exprboolset(KlGenUnit* gen, KlCst* cst, size_t target, bool setcond) {
  if (klcst_kind(cst) == KLCST_EXPR_PRE && klcast(KlCstPre*, cst)->op == KLTK_NOT) {
    KlCodeVal res = klgen_exprnot(gen, klcast(KlCstPre*, cst), setcond);
    return klgen_exprboolset_handleresult(gen, res, setcond);
  } else if (klcst_kind(cst) == KLCST_EXPR_BIN) {
    KlCstBin* bincst = klcast(KlCstBin*, cst);
    if (bincst->op == KLTK_AND)
      return klgen_exprandset(gen, bincst, target, setcond);
    if (bincst->op == KLTK_OR)
      return klgen_exprorset(gen, bincst, target, setcond);
    if (kltoken_isrelation(bincst->op)) {
      KlCodeVal res = klgen_exprrelation(gen, bincst, setcond);
      return klgen_exprboolset_handleresult(gen, res, setcond);
    }
    /* else is other binary expression, fallthrough */
  } else if (klcst_kind(cst) == KLCST_EXPR_TUPLE) {
    KlCstTuple* tuple = klcast(KlCstTuple*, cst);
    KlCst* lastelem = tuple->nelem == 0 ? NULL : tuple->elems[tuple->nelem - 1];
    if (lastelem && klcst_isboolexpr(lastelem = klgen_exprpromotion(lastelem))) {
      klgen_tuple_evaluate(gen, tuple, tuple->nelem - 1);
      return klgen_exprboolset(gen, lastelem, target, setcond);
    }
    /* else the tuple should be evaluated by klgen_expr, fallthrough */
  }
  KlCodeVal res = klgen_expr(gen, cst);
  if (klcodeval_isconstant(res)) return res;
  size_t stktop = klgen_stacktop(gen);
  klgen_putinstack(gen, &res, klgen_cstposition(cst));
  klgen_pushinst(gen, klinst_testset(target, res.index), klgen_cstposition(cst));
  klgen_pushinst(gen, klinst_condjmp(setcond, 0), klgen_cstposition(cst));
  klgen_mergejmp_maynone(gen, &gen->info.jumpinfo->terminatelist, klcodeval_jmp(klgen_pushinst(gen, klinst_jmp(0), klgen_cstposition(cst))));
  klgen_stackfree(gen, stktop);
  return klcodeval_none();
}

static void klgen_finishexprboolvalraw(KlGenUnit* gen, size_t target, KlFilePosition pos) {
  KlGenJumpInfo* jumpinfo = gen->info.jumpinfo;
  if (jumpinfo->truelist.kind != KLVAL_NONE || jumpinfo->falselist.kind != KLVAL_NONE) {
    size_t falsepos = klgen_pushinst(gen, klinst_loadfalseskip(target), pos);
    size_t truepos = klgen_pushinst(gen, klinst_loadbool(target, true), pos);
    klgen_setinstjmppos(gen, jumpinfo->falselist, falsepos);
    klgen_setinstjmppos(gen, jumpinfo->truelist, truepos);
  }
  klgen_setinstjmppos(gen, jumpinfo->terminatelist, klgen_currcodesize(gen));
  if (klgen_stacktop(gen) <= target)
    klgen_stackfree(gen, target + 1);
}

static KlCodeVal klgen_exprboolvalraw(KlGenUnit* gen, KlCst* cst, size_t target) {
  if (klcst_kind(cst) == KLCST_EXPR_PRE && klcast(KlCstPre*, cst)->op == KLTK_NOT) {
    KlCodeVal res = klgen_exprnot(gen, klcast(KlCstPre*, cst), true);
    if (klcodeval_isconstant(res)) return res;
    klgen_mergejmp_maynone(gen, &gen->info.jumpinfo->truelist, res);
    klgen_finishexprboolvalraw(gen, target, klgen_cstposition(cst));
    return klcodeval_none();
  } else if (klcst_kind(cst) == KLCST_EXPR_BIN) {
    KlCstBin* bincst = klcast(KlCstBin*, cst);
    if (bincst->op == KLTK_AND) {
      KlCodeVal ljmp = klgen_exprboolset(gen, bincst->loperand, target, false);
      if (klcodeval_isconstant(ljmp)) {
        if (klcodeval_isfalse(ljmp)) return ljmp;
        return klgen_exprboolvalraw(gen, bincst->roperand, target);
      }
      KlCodeVal rjmp = klgen_exprboolvalraw(gen, bincst->roperand, target);
      if (klcodeval_isconstant(rjmp)) {
        if (klcodeval_istrue(rjmp)) {
          rjmp = klcodeval_jmp(klgen_pushinst(gen, klinst_jmp(0), klgen_cstposition(bincst->roperand)));
          klgen_mergejmp_maynone(gen, &gen->info.jumpinfo->truelist, rjmp);
        }
        klgen_finishexprboolvalraw(gen, target, klgen_cstposition(cst));
      }
      return klcodeval_none();
    }
    if (bincst->op == KLTK_OR) {
      KlCodeVal ljmp = klgen_exprboolset(gen, bincst->loperand, target, true);
      if (klcodeval_isconstant(ljmp)) {
        if (klcodeval_istrue(ljmp)) return ljmp;
        return klgen_exprboolvalraw(gen, bincst->roperand, target);
      }
      KlCodeVal rjmp = klgen_exprboolvalraw(gen, bincst->roperand, target);
      if (klcodeval_isconstant(rjmp)) {
        if (klcodeval_istrue(rjmp)) {
          rjmp = klcodeval_jmp(klgen_pushinst(gen, klinst_jmp(0), klgen_cstposition(bincst->roperand)));
          klgen_mergejmp_maynone(gen, &gen->info.jumpinfo->truelist, rjmp);
        }
        klgen_finishexprboolvalraw(gen, target, klgen_cstposition(cst));
      }
      return klcodeval_none();
    }
    if (kltoken_isrelation(bincst->op)) {
      KlCodeVal res = klgen_exprrelation(gen, bincst, true);
      if (klcodeval_isconstant(res)) return res;
      klgen_mergejmp_maynone(gen, &gen->info.jumpinfo->truelist, res);
      klgen_finishexprboolvalraw(gen, target, klgen_cstposition(cst));
      return klcodeval_none();
    }
    /* else is other binary expression, fallthrough */
  } else if (klcst_kind(cst) == KLCST_EXPR_TUPLE) {
    KlCstTuple* tuple = klcast(KlCstTuple*, cst);
    KlCst* lastelem = tuple->nelem == 0 ? NULL : tuple->elems[tuple->nelem - 1];
    if (lastelem && klcst_isboolexpr(lastelem = klgen_exprpromotion(lastelem))) {
      klgen_tuple_evaluate(gen, tuple, tuple->nelem - 1);
      return klgen_exprboolvalraw(gen, lastelem, target);
    }
    /* else the tuple should be evaluated by klgen_expr, fallthrough */
  }
  KlCodeVal res = klgen_exprtarget(gen, cst, target);
  if (klcodeval_isconstant(res)) return res;
  if (gen->info.jumpinfo->truelist.kind == KLVAL_NONE &&
      gen->info.jumpinfo->falselist.kind == KLVAL_NONE) {
    if (gen->info.jumpinfo->terminatelist.kind != KLVAL_NONE)
      klgen_setinstjmppos(gen, gen->info.jumpinfo->terminatelist, klgen_currcodesize(gen));
  } else {
    KlCodeVal jmp = klcodeval_jmp(klgen_pushinst(gen, klinst_jmp(0), klgen_cstposition(cst)));
    klgen_mergejmp_maynone(gen, &gen->info.jumpinfo->terminatelist, jmp);
    klgen_finishexprboolvalraw(gen, target, klgen_cstposition(cst));
  }
  return klcodeval_none();
}

KlCodeVal klgen_exprboolval(KlGenUnit* gen, KlCst* cst, size_t target) {
  KlGenJumpInfo jumpinfo;
  jumpinfo.truelist = klcodeval_none();
  jumpinfo.falselist = klcodeval_none();
  jumpinfo.terminatelist = klcodeval_none();
  /* push new jumpinfo structure */
  jumpinfo.prev = gen->info.jumpinfo;
  gen->info.jumpinfo = &jumpinfo;
  KlCodeVal res = klgen_exprboolvalraw(gen, cst, target);
  /* pop jumpinfo structure */
  gen->info.jumpinfo = jumpinfo.prev;
  return res;
}
