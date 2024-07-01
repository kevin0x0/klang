#include "include/code/klgen_exprbool.h"
#include "include/ast/klast.h"
#include "include/code/klcode.h"
#include "include/code/klcodeval.h"
#include "include/code/klgen.h"
#include "include/code/klgen_expr.h"
#include "include/code/klgen_stmt.h"
#include <string.h>


static KlCodeVal klgen_exprnot(KlGenUnit* gen, KlAstPre* notast, bool jumpcond);
static KlCodeVal klgen_expror(KlGenUnit* gen, KlAstBin* orast, bool jumpcond);
static KlCodeVal klgen_exprand(KlGenUnit* gen, KlAstBin* andast, bool jumpcond);
static KlCodeVal klgen_exprrelrightnonstk(KlGenUnit* gen, KlAstBin* binast, KlCStkId oristktop, KlCodeVal left, KlCodeVal right, bool jumpcond);
static KlCodeVal klgen_exprboolset(KlGenUnit* gen, KlAst* boolast, KlCStkId target, bool setcond);

static inline int klgen_getoffset(KlInstruction jmpinst) {
  if (KLINST_GET_OPCODE(jmpinst) == KLOPCODE_JMP) {
    return KLINST_I_GETI(jmpinst);
  } else {
    return KLINST_AI_GETI(jmpinst);
  }
}

void klgen_jumpto(KlGenUnit* gen, KlCodeVal jmplist, KlCPC jmppos) {
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

static KlCodeVal klgen_pushrelinst(KlGenUnit* gen, KlAstBin* relast, KlCStkId leftid, KlCStkId rightid, bool jumpcond) {
  switch (relast->op) {
    case KLTK_LT: {
      klgen_emit(gen, klinst_lt(leftid, rightid), klgen_astposition(relast));
      break;
    }
    case KLTK_LE: {
      klgen_emit(gen, klinst_le(leftid, rightid), klgen_astposition(relast));
      break;
    }
    case KLTK_GT: {
      klgen_emit(gen, klinst_gt(leftid, rightid), klgen_astposition(relast));
      break;
    }
    case KLTK_GE: {
      klgen_emit(gen, klinst_ge(leftid, rightid), klgen_astposition(relast));
      break;
    }
    case KLTK_EQ: {
      klgen_emit(gen, klinst_eq(leftid, rightid), klgen_astposition(relast));
      break;
    }
    case KLTK_IS: {
      klgen_emit(gen, klinst_is(leftid, rightid), klgen_astposition(relast));
      break;
    }
    case KLTK_NE: {
      klgen_emit(gen, klinst_ne(leftid, rightid), klgen_astposition(relast));
      break;
    }
    case KLTK_ISNOT: {
      klgen_emit(gen, klinst_is(leftid, rightid), klgen_astposition(relast));
      jumpcond = !jumpcond;
      break;
    }
    default: {
      kl_assert(false, "control flow should not reach here");
      break;
    }
  }
  KlCPC pc = klgen_emit(gen, klinst_condjmp(jumpcond, 0), klgen_astposition(relast));
  return klcodeval_jmplist(pc);
}

static KlCodeVal klgen_pushrelinsti(KlGenUnit* gen, KlAstBin* relast, KlCStkId leftid, KlCInt imm, bool jumpcond) {
  switch (relast->op) {
    case KLTK_LT: {
      klgen_emit(gen, klinst_lti(leftid, imm), klgen_astposition(relast));
      break;
    }
    case KLTK_LE: {
      klgen_emit(gen, klinst_lei(leftid, imm), klgen_astposition(relast));
      break;
    }
    case KLTK_GT: {
      klgen_emit(gen, klinst_gti(leftid, imm), klgen_astposition(relast));
      break;
    }
    case KLTK_GE: {
      klgen_emit(gen, klinst_gei(leftid, imm), klgen_astposition(relast));
      break;
    }
    case KLTK_EQ: {
      if (jumpcond) {
        klgen_emit(gen, klinst_eqi(leftid, imm), klgen_astposition(relast));
      } else {
        klgen_emit(gen, klinst_nei(leftid, imm), klgen_astposition(relast));
      }
      KlCPC pc = klgen_emit(gen, klinst_condjmp(true, 0), klgen_astposition(relast));
      return klcodeval_jmplist(pc);
    }
    case KLTK_NE: {
      if (jumpcond) {
        klgen_emit(gen, klinst_nei(leftid, imm), klgen_astposition(relast));
      } else {
        klgen_emit(gen, klinst_eqi(leftid, imm), klgen_astposition(relast));
      }
      KlCPC pc = klgen_emit(gen, klinst_condjmp(true, 0), klgen_astposition(relast));
      return klcodeval_jmplist(pc);
    }
    default: {
      kl_assert(false, "control flow should not reach here");
      break;
    }
  }
  KlCPC pc = klgen_emit(gen, klinst_condjmp(jumpcond, 0), klgen_astposition(relast));
  return klcodeval_jmplist(pc);
}

static KlCodeVal klgen_pushrelinstc(KlGenUnit* gen, KlAstBin* relast, KlCStkId leftid, KlCIdx conidx, bool jumpcond) {
  switch (relast->op) {
    case KLTK_LT: {
      klgen_emit(gen, klinst_ltc(leftid, conidx), klgen_astposition(relast));
      break;
    }
    case KLTK_LE: {
      klgen_emit(gen, klinst_lec(leftid, conidx), klgen_astposition(relast));
      break;
    }
    case KLTK_GT: {
      klgen_emit(gen, klinst_gtc(leftid, conidx), klgen_astposition(relast));
      break;
    }
    case KLTK_GE: {
      klgen_emit(gen, klinst_gec(leftid, conidx), klgen_astposition(relast));
      break;
    }
    case KLTK_EQ: {
      if (jumpcond) {
        klgen_emit(gen, klinst_eqc(leftid, conidx), klgen_astposition(relast));
      } else {
        klgen_emit(gen, klinst_nec(leftid, conidx), klgen_astposition(relast));
      }
      KlCPC pc = klgen_emit(gen, klinst_condjmp(true, 0), klgen_astposition(relast));
      return klcodeval_jmplist(pc);
    }
    case KLTK_NE: {
      if (jumpcond) {
        klgen_emit(gen, klinst_nec(leftid, conidx), klgen_astposition(relast));
      } else {
        klgen_emit(gen, klinst_eqc(leftid, conidx), klgen_astposition(relast));
      }
      KlCPC pc = klgen_emit(gen, klinst_condjmp(true, 0), klgen_astposition(relast));
      return klcodeval_jmplist(pc);
    }
    default: {
      kl_assert(false, "control flow should not reach here");
      break;
    }
  }
  KlCPC pc = klgen_emit(gen, klinst_condjmp(jumpcond, 0), klgen_astposition(relast));
  return klcodeval_jmplist(pc);
}

static KlCodeVal klgen_relcomptime(KlGenUnit* gen, KlAstBin* binast, KlCodeVal left, KlCodeVal right) {
  if (left.kind == KLVAL_INTEGER && right.kind == KLVAL_INTEGER) {
    KlCBool cond;
    switch (binast->op) {
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
      cmpres = strncmp(klstrtbl_getstring(gen->strtbl, left.string.id),
                       klstrtbl_getstring(gen->strtbl, right.string.id),
                       left.string.length);
    } else {
      cmpres = left.string.length > right.string.length ? 1 : -1;
    }
    KlCBool cond;
    switch (binast->op) {
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
    KlCFloat l = left.kind == KLVAL_INTEGER ? (KlCFloat)left.intval : left.floatval;
    KlCFloat r = right.kind == KLVAL_INTEGER ? (KlCFloat)right.intval : right.floatval;
    KlCBool cond;
    switch (binast->op) {
      case KLTK_LT: cond = l < r; break;
      case KLTK_LE: cond = l <= r; break;
      case KLTK_GT: cond = l > r; break;
      case KLTK_GE: cond = l >= r; break;
      case KLTK_ISNOT: cond = left.kind != right.kind || l != r; break;
      case KLTK_NE: cond = l != r; break;
      case KLTK_EQ: cond = l == r; break;
      case KLTK_IS: cond = left.kind == right.kind && l == r; break;
      default: {
        kl_assert(false, "control flow should not reach here");
        return klcodeval_nil();
      }
    }
    return klcodeval_bool(cond);
  }
}

static KlCodeVal klgen_equalitycomptime(KlGenUnit* gen, KlAstBin* binast, KlCodeVal left, KlCodeVal right) {
  kl_assert(klcodeval_isconstant(left) && klcodeval_isconstant(right), "");
  if (left.kind != right.kind) {
    return klcodeval_bool(binast->op == KLTK_NE);
  } else {
    switch (left.kind) {
      case KLVAL_STRING:
      case KLVAL_FLOAT:
      case KLVAL_INTEGER: return klgen_relcomptime(gen, binast, left, right);
      case KLVAL_NIL: return klcodeval_bool(binast->op == KLTK_EQ || binast->op == KLTK_IS);
      case KLVAL_BOOL: return klcodeval_bool(binast->op == KLTK_EQ || binast->op == KLTK_IS ? left.boolval == right.boolval : left.boolval != right.boolval);
      default: {
        kl_assert(false, "control flow should not reach here");
        return klcodeval_nil();
      }
    }
  }
}

static KlCodeVal klgen_exprrelleftliteral(KlGenUnit* gen, KlAstBin* binast, KlCodeVal left, bool jumpcond) {
  /* left is not on the stack, so the stack top is not changed */
  KlCStkId oristktop = klgen_stacktop(gen);
  KlCodeVal right = klgen_expr(gen, binast->roperand);
  if ((klcodeval_isnumber(right) && klcodeval_isnumber(left)) ||
      (right.kind == KLVAL_STRING && right.kind == KLVAL_STRING)) {
    kl_assert(oristktop == klgen_stacktop(gen), "");
    /* do compile time comparison */
    return klgen_relcomptime(gen, binast, left, right);
  } else if (binast->op == KLTK_NE || binast->op == KLTK_EQ ||
             binast->op == KLTK_IS || binast->op == KLTK_ISNOT) {
    if (klcodeval_isconstant(right)) {
      kl_assert(oristktop == klgen_stacktop(gen), "");
      return klgen_equalitycomptime(gen, binast, left, right);
    }
  } /* else can not apply compile time comparison */
  klgen_putonstack(gen, &left, klgen_astposition(binast->loperand));
  if (right.kind == KLVAL_STACK) {
    klgen_stackfree(gen, oristktop);
    return klgen_pushrelinst(gen, binast, left.index, right.index, jumpcond);
  } else {
    return klgen_exprrelrightnonstk(gen, binast, oristktop, left, right, jumpcond);
  }
}

static KlCodeVal klgen_exprrelrightnonstk(KlGenUnit* gen, KlAstBin* binast, KlCStkId oristktop, KlCodeVal left, KlCodeVal right, bool jumpcond) {
  /* left must be on stack */
  kl_assert(left.kind == KLVAL_STACK, "");
  if (binast->op == KLTK_IS || binast->op == KLTK_ISNOT) {
    klgen_putonstack(gen, &right, klgen_astposition(binast->roperand));
    klgen_stackfree(gen, oristktop);
    return klgen_pushrelinst(gen, binast, left.index, right.index, jumpcond);
  }
  switch (right.kind) {
    case KLVAL_INTEGER: {
      KlCodeVal res = klinst_inrange(right.intval, 16)
                    ? klgen_pushrelinsti(gen, binast, left.index, right.intval, jumpcond)
                    : klgen_pushrelinstc(gen, binast, left.index, klgen_newinteger(gen, right.intval), jumpcond);
      klgen_stackfree(gen, oristktop);
      return res;
    }
    case KLVAL_FLOAT: {
      KlCodeVal res = klgen_pushrelinstc(gen, binast, left.index, klgen_newfloat(gen, right.floatval), jumpcond);
      klgen_stackfree(gen, oristktop);
      return res;
    }
    case KLVAL_STRING: {
      KlCodeVal res = klgen_pushrelinstc(gen, binast, left.index, klgen_newstring(gen, right.string), jumpcond);
      klgen_stackfree(gen, oristktop);
      return res;
    }
    case KLVAL_BOOL: {
      KlCodeVal res = klgen_pushrelinstc(gen, binast, left.index,
                                         klgen_newconstant(gen, &(KlConstant) {.type = KLC_BOOL, .boolval = right.boolval }), jumpcond);
      klgen_stackfree(gen, oristktop);
      return res;
    }
    case KLVAL_NIL: {
      KlCodeVal res = klgen_pushrelinstc(gen, binast, left.index, klgen_newconstant(gen, &(KlConstant) { .type = KLC_NIL }), jumpcond);
      klgen_stackfree(gen, oristktop);
      return res;
    }
    case KLVAL_REF: {
      klgen_putonstack(gen, &right, klgen_astposition(binast->roperand));
      klgen_stackfree(gen, oristktop);
      return klgen_pushrelinst(gen, binast, left.index, right.index, jumpcond);
    }
    default: {
      kl_assert(false, "control flow should not reach here");
      return klcodeval_nil();
    }
  }
}

static KlCodeVal klgen_exprrelation(KlGenUnit* gen, KlAstBin* relast, bool jumpcond) {
  KlCStkId oristktop = klgen_stacktop(gen);
  KlCodeVal left = klgen_expr(gen, relast->loperand);
  if (klcodeval_isconstant(left)) {
    return klgen_exprrelleftliteral(gen, relast, left, jumpcond);
  } else {
    klgen_putonstack(gen, &left, klgen_astposition(relast->loperand));
  }
  /* now left is on stack */
  KlCodeVal right = klgen_expr(gen, relast->roperand);
  if (right.kind != KLVAL_STACK)
    return klgen_exprrelrightnonstk(gen, relast, oristktop, left, right, jumpcond);
  /* now both are on stack */
  klgen_stackfree(gen, oristktop);
  return klgen_pushrelinst(gen, relast, left.index, right.index, jumpcond);
}

static KlCodeVal klgen_exprwhere_ascondition(KlGenUnit* gen, KlAstWhere* whereast, bool jumpcond) {
  size_t oristktop = klgen_stacktop(gen);
  klgen_pushsymtbl(gen);
  klgen_stmtlist(gen, whereast->block);
  KlCodeVal jmplist = klgen_exprbool(gen, whereast->expr, jumpcond);
  bool referenced = gen->symtbl->info.referenced;
  klgen_popsymtbl(gen);
  klgen_stackfree(gen, oristktop);

  if (klcodeval_isconstant(jmplist)) {
    if (referenced)
      klgen_emit(gen, klinst_close(oristktop), klgen_astposition(whereast));
    return jmplist;
  }
  if (!referenced) return jmplist;
  klgen_emit(gen, klinst_closejmp(oristktop, 1), klgen_astposition(whereast));
  klgen_jumpto(gen, jmplist, klgen_getjmptarget(gen));
  return klcodeval_jmplist(klgen_emit(gen, klinst_closejmp(oristktop, 0), klgen_astposition(whereast)));
}


KlCodeVal klgen_exprbool(KlGenUnit* gen, KlAst* ast, bool jumpcond) {
  if (klast_kind(ast) == KLAST_EXPR_PRE && klcast(KlAstPre*, ast)->op == KLTK_NOT) {
    return klgen_exprnot(gen, klcast(KlAstPre*, ast), jumpcond);
  } else if (klast_kind(ast) == KLAST_EXPR_BIN) {
    KlAstBin* binast = klcast(KlAstBin*, ast);
    if (binast->op == KLTK_AND)
      return klgen_exprand(gen, binast, jumpcond);
    if (binast->op == KLTK_OR)
      return klgen_expror(gen, binast, jumpcond);
    if (kltoken_isrelation(binast->op))
      return klgen_exprrelation(gen, binast, jumpcond);
    /* else is other binary expression, fallthrough */
  } else if (klast_kind(ast) == KLAST_EXPR_WHERE && klast_isboolexpr(klcast(KlAstWhere*, ast)->expr)) {
    return klgen_exprwhere_ascondition(gen, klcast(KlAstWhere*, ast), jumpcond);
  } else if (klast_kind(ast) == KLAST_EXPR_LIST) {
    KlAstExprList* exprlist = klcast(KlAstExprList*, ast);
    KlAst* lastelem = exprlist->nexpr == 0 ? NULL : exprlist->exprs[exprlist->nexpr - 1];
    if (lastelem && klast_isboolexpr(lastelem)) {
      klgen_exprlist_raw(gen, exprlist->exprs, exprlist->nexpr - 1, 0, klgen_astposition(exprlist));
      return klgen_exprbool(gen, lastelem, jumpcond);
    }
    /* else the exprlist should be evaluated by klgen_expr, fallthrough */
  }
  KlCStkId stktop = klgen_stacktop(gen);
  KlCodeVal res = klgen_expr(gen, ast);
  if (klcodeval_isconstant(res)) return res;
  klgen_putonstack(gen, &res, klgen_astposition(ast));
  KlCPC pc = klgen_emit(gen, jumpcond ? klinst_truejmp(res.index, 0) : klinst_falsejmp(res.index, 0), klgen_astposition(ast));
  klgen_stackfree(gen, stktop);
  return klcodeval_jmplist(pc);
}

KlCodeVal klgen_exprnot(KlGenUnit* gen, KlAstPre* notast, bool jumpcond) {
  KlCodeVal jmp = klgen_exprbool(gen, notast->operand, !jumpcond);
  if (klcodeval_isconstant(jmp))
    return klcodeval_bool(klcodeval_isfalse(jmp));
  return jmp;
}

KlCodeVal klgen_expror(KlGenUnit* gen, KlAstBin* orast, bool jumpcond) {
  KlCodeVal ljmp = klgen_exprbool(gen, orast->loperand, true);
  if (klcodeval_isconstant(ljmp)) {
    if (klcodeval_istrue(ljmp)) return ljmp;
    return klgen_exprbool(gen, orast->roperand, jumpcond);
  }
  KlCodeVal rjmp = klgen_exprbool(gen, orast->roperand, jumpcond);
  if (klcodeval_isconstant(rjmp)) {
    KlCStkId stktop = klgen_stacktop(gen);
    klgen_putonstack(gen, &rjmp, klgen_astposition(orast->roperand));
    rjmp = klcodeval_jmplist(klgen_emit(gen,
                                        jumpcond ? klinst_truejmp(rjmp.index, 0)
                                                 : klinst_falsejmp(rjmp.index, 0),
                                        klgen_astposition(orast->roperand)));
    klgen_stackfree(gen, stktop);
  }
  if (jumpcond) {
    return klgen_mergejmplist(gen, ljmp, rjmp);
  } else {
    klgen_jumpto(gen, ljmp, klgen_getjmptarget(gen));
    return rjmp;
  }
}

KlCodeVal klgen_exprand(KlGenUnit* gen, KlAstBin* andast, bool jumpcond) {
  KlCodeVal ljmp = klgen_exprbool(gen, andast->loperand, false);
  if (klcodeval_isconstant(ljmp)) {
    if (klcodeval_isfalse(ljmp)) return ljmp;
    return klgen_exprbool(gen, andast->roperand, jumpcond);
  }
  KlCodeVal rjmp = klgen_exprbool(gen, andast->roperand, jumpcond);
  if (klcodeval_isconstant(rjmp)) {
    KlCStkId stktop = klgen_stacktop(gen);
    klgen_putonstack(gen, &rjmp, klgen_astposition(andast->roperand));
    rjmp = klcodeval_jmplist(klgen_emit(gen,
                                        jumpcond ? klinst_truejmp(rjmp.index, 0)
                                                 : klinst_falsejmp(rjmp.index, 0),
                                        klgen_astposition(andast->roperand)));
    klgen_stackfree(gen, stktop);
  }
  if (jumpcond) {
    klgen_jumpto(gen, ljmp, klgen_getjmptarget(gen));
    return rjmp;
  } else {
    return klgen_mergejmplist(gen, ljmp, rjmp);
  }
}

static KlCodeVal klgen_exprorset(KlGenUnit* gen, KlAstBin* orast, KlCStkId target, bool setcond) {
  if (setcond) {
    KlCodeVal lval = klgen_exprboolset(gen, orast->loperand, target, true);
    if (klcodeval_isconstant(lval)) {
      if (klcodeval_istrue(lval)) return lval;
      return klgen_exprboolset(gen, orast->roperand, target, setcond);
    }
    KlCodeVal rval = klgen_exprboolset(gen, orast->roperand, target, setcond);
    if (klcodeval_istrue(rval)) { /* true, set value and jump over */
      KlCStkId stktop = klgen_stacktop(gen);
      klgen_loadval(gen, target, rval, klgen_astposition(orast->roperand));
      KlCPC pc = klgen_emit(gen, klinst_jmp(0), klgen_astposition(orast->roperand));
      klgen_mergejmplist_maynone(gen, &gen->jmpinfo.jumpinfo->terminatelist, klcodeval_jmplist(pc));
      klgen_stackfree(gen, stktop);
    }
    /* else false, fall through */
    return klcodeval_none();
  } else {
    KlCodeVal ljmp = klgen_exprbool(gen, orast->loperand, true);
    if (klcodeval_isconstant(ljmp)) {
      if (klcodeval_istrue(ljmp)) return ljmp;
      return klgen_exprboolset(gen, orast->roperand, target, setcond);
    }
    KlCodeVal rval = klgen_exprboolset(gen, orast->roperand, target, setcond);
    if (klcodeval_isfalse(rval)) { /* false, set value and jump over */
      KlCStkId stktop = klgen_stacktop(gen);
      klgen_loadval(gen, target, rval, klgen_astposition(orast->roperand));
      KlCPC pc = klgen_emit(gen, klinst_jmp(0), klgen_astposition(orast->roperand));
      klgen_mergejmplist_maynone(gen, &gen->jmpinfo.jumpinfo->terminatelist, klcodeval_jmplist(pc));
      klgen_stackfree(gen, stktop);
    }
    /* else true, fall through */
    klgen_jumpto(gen, ljmp, klgen_getjmptarget(gen));
    return klcodeval_none();
  }
}

static KlCodeVal klgen_exprandset(KlGenUnit* gen, KlAstBin* andast, KlCStkId target, bool setcond) {
  if (setcond) {
    KlCodeVal ljmp = klgen_exprbool(gen, andast->loperand, false);
    if (klcodeval_isconstant(ljmp)) {
      if (klcodeval_isfalse(ljmp)) return ljmp;
      return klgen_exprboolset(gen, andast->roperand, target, setcond);
    }
    KlCodeVal rval = klgen_exprboolset(gen, andast->roperand, target, setcond);
    if (klcodeval_istrue(rval)) { /* true, set value and jump over */
      KlCStkId stktop = klgen_stacktop(gen);
      klgen_loadval(gen, target, rval, klgen_astposition(andast->roperand));
      KlCPC pc = klgen_emit(gen, klinst_jmp(0), klgen_astposition(andast->roperand));
      klgen_mergejmplist_maynone(gen, &gen->jmpinfo.jumpinfo->terminatelist, klcodeval_jmplist(pc));
      klgen_stackfree(gen, stktop);
    }
    /* else false, fall through */
    klgen_jumpto(gen, ljmp, klgen_getjmptarget(gen));
    return klcodeval_none();
  } else {
    KlCodeVal lval = klgen_exprboolset(gen, andast->loperand, target, false);
    if (klcodeval_isconstant(lval)) {
      if (klcodeval_isfalse(lval)) return lval;
      return klgen_exprboolset(gen, andast->roperand, target, setcond);
    }
    KlCodeVal rval = klgen_exprboolset(gen, andast->roperand, target, setcond);
    if (klcodeval_isfalse(rval)) { /* false, set value and jump over */
      KlCStkId stktop = klgen_stacktop(gen);
      klgen_loadval(gen, target, rval, klgen_astposition(andast->roperand));
      KlCPC pc = klgen_emit(gen, klinst_jmp(0), klgen_astposition(andast->roperand));
      klgen_mergejmplist_maynone(gen, &gen->jmpinfo.jumpinfo->terminatelist, klcodeval_jmplist(pc));
      klgen_stackfree(gen, stktop);
    }
    /* else true, fall through */
    return klcodeval_none();
  }
}

static KlCodeVal klgen_exprboolset_handleresult(KlGenUnit* gen, KlCodeVal res, bool setcond) {
  if (klcodeval_isconstant(res)) return res;
  KlCodeVal* jmplist = setcond ? &gen->jmpinfo.jumpinfo->truelist : &gen->jmpinfo.jumpinfo->falselist;
  klgen_mergejmplist_maynone(gen, jmplist, res);
  return klcodeval_none();
}

static KlCodeVal klgen_exprboolset(KlGenUnit* gen, KlAst* ast, KlCStkId target, bool setcond) {
  if (klast_kind(ast) == KLAST_EXPR_PRE && klcast(KlAstPre*, ast)->op == KLTK_NOT) {
    KlCodeVal res = klgen_exprnot(gen, klcast(KlAstPre*, ast), setcond);
    return klgen_exprboolset_handleresult(gen, res, setcond);
  } else if (klast_kind(ast) == KLAST_EXPR_BIN) {
    KlAstBin* binast = klcast(KlAstBin*, ast);
    if (binast->op == KLTK_AND)
      return klgen_exprandset(gen, binast, target, setcond);
    if (binast->op == KLTK_OR)
      return klgen_exprorset(gen, binast, target, setcond);
    if (kltoken_isrelation(binast->op)) {
      KlCodeVal res = klgen_exprrelation(gen, binast, setcond);
      return klgen_exprboolset_handleresult(gen, res, setcond);
    }
    /* else is other binary expression, fallthrough */
  } else if (klast_kind(ast) == KLAST_EXPR_LIST) {
    KlAstExprList* exprlist = klcast(KlAstExprList*, ast);
    KlAst* lastelem = exprlist->nexpr == 0 ? NULL : exprlist->exprs[exprlist->nexpr - 1];
    if (lastelem && klast_isboolexpr(lastelem)) {
      klgen_exprlist_raw(gen, exprlist->exprs, exprlist->nexpr - 1, 0, klgen_astposition(exprlist));
      return klgen_exprboolset(gen, lastelem, target, setcond);
    }
    /* else the exprlist should be evaluated by klgen_expr, fallthrough */
  }
  KlCStkId stktop = klgen_stacktop(gen);
  KlCodeVal res = klgen_expr(gen, ast);
  if (klcodeval_isconstant(res)) return res;
  klgen_putonstack(gen, &res, klgen_astposition(ast));
  KlCPC terminatepc;
  if (target == res.index) {
    terminatepc = klgen_emit(gen, setcond ? klinst_truejmp(target, 0) : klinst_falsejmp(target, 0), klgen_astposition(ast));
  } else {
    klgen_emit(gen, klinst_testset(target, res.index), klgen_astposition(ast));
    terminatepc = klgen_emit(gen, klinst_condjmp(setcond, 0), klgen_astposition(ast));
  }
  klgen_mergejmplist_maynone(gen, &gen->jmpinfo.jumpinfo->terminatelist, klcodeval_jmplist(terminatepc));
  klgen_stackfree(gen, stktop);
  return klcodeval_none();
}

static void klgen_finishexprboolvalraw(KlGenUnit* gen, KlCStkId target, KlFilePosition pos) {
  KlGenJumpInfo* jumpinfo = gen->jmpinfo.jumpinfo;
  klgen_jumpto(gen, jumpinfo->falselist, klgen_getjmptarget(gen));
  klgen_emit(gen, klinst_loadfalseskip(target), pos);
  klgen_jumpto(gen, jumpinfo->truelist, klgen_getjmptarget(gen));
  klgen_emit(gen, klinst_loadbool(target, true), pos);
  klgen_jumpto(gen, jumpinfo->terminatelist, klgen_getjmptarget(gen));
}

static void klgen_finishexprboolvalraw_afterload(KlGenUnit* gen, KlCStkId target, KlFilePosition pos) {
  if (gen->jmpinfo.jumpinfo->truelist.kind == KLVAL_NONE &&
      gen->jmpinfo.jumpinfo->falselist.kind == KLVAL_NONE) {
    klgen_jumpto(gen, gen->jmpinfo.jumpinfo->terminatelist, klgen_getjmptarget(gen));
  } else {
    KlCodeVal jmp = klcodeval_jmplist(klgen_emit(gen, klinst_jmp(0), pos));
    klgen_mergejmplist_maynone(gen, &gen->jmpinfo.jumpinfo->terminatelist, jmp);
    klgen_finishexprboolvalraw(gen, target, pos);
  }
}

static KlCodeVal klgen_exprboolvalraw(KlGenUnit* gen, KlAst* ast, KlCStkId target) {
  if (klast_kind(ast) == KLAST_EXPR_PRE && klcast(KlAstPre*, ast)->op == KLTK_NOT) {
    KlCodeVal res = klgen_exprnot(gen, klcast(KlAstPre*, ast), true);
    if (klcodeval_isconstant(res)) return res;
    klgen_mergejmplist_maynone(gen, &gen->jmpinfo.jumpinfo->truelist, res);
    klgen_finishexprboolvalraw(gen, target, klgen_astposition(ast));
    klgen_stackfree(gen, target < klgen_stacktop(gen) ? klgen_stacktop(gen) : target + 1);
    return klcodeval_none();
  } else if (klast_kind(ast) == KLAST_EXPR_BIN) {
    KlAstBin* binast = klcast(KlAstBin*, ast);
    if (binast->op == KLTK_AND) {
      KlCodeVal ljmp = klgen_exprboolset(gen, binast->loperand, target, false);
      if (klcodeval_isconstant(ljmp)) {
        if (klcodeval_isfalse(ljmp)) return ljmp;
        return klgen_exprboolvalraw(gen, binast->roperand, target);
      }
      KlCodeVal rjmp = klgen_exprboolvalraw(gen, binast->roperand, target);
      if (klcodeval_isconstant(rjmp)) {
        klgen_loadval(gen, target, rjmp, klgen_astposition(binast->roperand));
        klgen_finishexprboolvalraw_afterload(gen, target, klgen_astposition(ast));
        klgen_stackfree(gen, target < klgen_stacktop(gen) ? klgen_stacktop(gen) : target + 1);
      }
      return klcodeval_none();
    } else if (binast->op == KLTK_OR) {
      KlCodeVal ljmp = klgen_exprboolset(gen, binast->loperand, target, true);
      if (klcodeval_isconstant(ljmp)) {
        if (klcodeval_istrue(ljmp)) return ljmp;
        return klgen_exprboolvalraw(gen, binast->roperand, target);
      }
      KlCodeVal rjmp = klgen_exprboolvalraw(gen, binast->roperand, target);
      if (klcodeval_isconstant(rjmp)) {
        klgen_loadval(gen, target, rjmp, klgen_astposition(binast->roperand));
        klgen_finishexprboolvalraw_afterload(gen, target, klgen_astposition(ast));
        klgen_stackfree(gen, target < klgen_stacktop(gen) ? klgen_stacktop(gen) : target + 1);
      }
      return klcodeval_none();
    } else if (kltoken_isrelation(binast->op)) {
      KlCodeVal res = klgen_exprrelation(gen, binast, true);
      if (klcodeval_isconstant(res)) return res;
      klgen_mergejmplist_maynone(gen, &gen->jmpinfo.jumpinfo->truelist, res);
      klgen_finishexprboolvalraw(gen, target, klgen_astposition(ast));
      klgen_stackfree(gen, target < klgen_stacktop(gen) ? klgen_stacktop(gen) : target + 1);
      return klcodeval_none();
    }
    /* else is other binary expression, fallthrough */
  } else if (klast_kind(ast) == KLAST_EXPR_LIST) {
    KlAstExprList* exprlist = klcast(KlAstExprList*, ast);
    KlAst* lastelem = exprlist->nexpr == 0 ? NULL : exprlist->exprs[exprlist->nexpr - 1];
    if (lastelem && klast_isboolexpr(lastelem)) {
      klgen_exprlist_raw(gen, exprlist->exprs, exprlist->nexpr - 1, 0, klgen_astposition(exprlist));
      return klgen_exprboolvalraw(gen, lastelem, target);
    }
    /* else the exprlist should be evaluated by klgen_expr, fallthrough */
  }
  KlCStkId stktop = klgen_stacktop(gen);
  KlCodeVal res = klgen_exprtarget(gen, ast, target);
  if (klcodeval_isconstant(res)) return res;
  klgen_finishexprboolvalraw_afterload(gen, target, klgen_astposition(ast));
  klgen_stackfree(gen, target < stktop ? stktop : target + 1);
  return klcodeval_none();
}

KlCodeVal klgen_exprboolval(KlGenUnit* gen, KlAst* ast, KlCStkId target) {
  KlGenJumpInfo jumpinfo;
  jumpinfo.truelist = klcodeval_none();
  jumpinfo.falselist = klcodeval_none();
  jumpinfo.terminatelist = klcodeval_none();
  /* push new jumpinfo structure */
  jumpinfo.prev = gen->jmpinfo.jumpinfo;
  gen->jmpinfo.jumpinfo = &jumpinfo;
  KlCodeVal res = klgen_exprboolvalraw(gen, ast, target);
  /* pop jumpinfo structure */
  gen->jmpinfo.jumpinfo = jumpinfo.prev;
  return res;
}
