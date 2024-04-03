#include "klang/include/code/klgen_stmt.h"
#include "klang/include/code/klcodeval.h"
#include "klang/include/code/klcontbl.h"
#include "klang/include/code/klgen.h"
#include "klang/include/code/klgen_expr.h"
#include "klang/include/cst/klcst.h"
#include "klang/include/cst/klcst_expr.h"
#include "klang/include/cst/klcst_stmt.h"

static void klgen_stmtlet(KlGenUnit* gen, KlCstStmtLet* letcst) {
  size_t stkid = klgen_stacktop(gen);
  klgen_tuple(gen, klcast(KlCstTuple*, letcst->rvals), letcst->nlval);
  kl_assert(stkid + letcst->nlval == klgen_stacktop(gen), "");
  KlStrDesc* ids = letcst->lvals;
  size_t nlval = letcst->nlval;
  for (size_t i = 0; i < nlval; ++i)
    klgen_newsymbol(gen, ids[i], stkid + i, klcst_begin(letcst));
}

static void klgen_singleassign(KlGenUnit* gen, KlCst* lval, KlCst* rval) {
  if (klcst_kind(lval) == KLCST_EXPR_ID) {
    KlCstIdentifier* id = klcast(KlCstIdentifier*, lval);
    KlSymbol* symbol = klgen_getsymbol(gen, id->id);
    if (!symbol) {  /* global variable */
      size_t stktop = klgen_stacktop(gen);
      KlCodeVal res = klgen_expr(gen, rval);
      klgen_putinstack(gen, &res, klgen_cstposition(rval));
      KlConstant constant = { .type = KL_STRING, .string = id->id };
      KlConEntry* conent = klcontbl_get(gen->contbl, &constant);
      klgen_oomifnull(conent);
      klgen_pushinst(gen, klinst_storeglobal(res.index, conent->index), klgen_cstposition(lval));
      klgen_stackfree(gen, stktop);
    } else if (symbol->attr.kind == KLVAL_REF) {
      size_t stktop = klgen_stacktop(gen);
      KlCodeVal res = klgen_expr(gen, rval);
      klgen_putinstack(gen, &res, klgen_cstposition(rval));
      klgen_pushinst(gen, klinst_storeref(res.index, symbol->attr.idx), klgen_cstposition(lval));
      klgen_stackfree(gen, stktop);
    } else {
      kl_assert(symbol->attr.kind == KLVAL_STACK, "");
      klgen_exprtarget(gen, rval, symbol->attr.idx);
    }
  } else if (klcst_kind(lval) == KLCST_EXPR_POST && klcast(KlCstPost*, lval)->op == KLTK_INDEX) {
    size_t stktop = klgen_stacktop(gen);
    KlCstPost* indexcst = klcast(KlCstPost*, lval);
    KlCst* indexablecst = indexcst->operand;
    KlCst* keycst = indexcst->post;
    KlCodeVal indexable = klgen_expr(gen, indexablecst);
    klgen_putinstack(gen, &indexable, klgen_cstposition(indexablecst));
    KlCodeVal key = klgen_expr(gen, keycst);
    if (key.kind != KLVAL_INTEGER || klinst_inrange(key.intval, 8))
      klgen_putinstack(gen, &key, klgen_cstposition(keycst));
    KlCodeVal val = klgen_expr(gen, rval);
    klgen_putinstack(gen, &val, klgen_cstposition(rval));
    if (key.kind == KLVAL_INTEGER) {
      klgen_pushinst(gen, klinst_indexasi(val.index, indexable.index, key.intval), klgen_cstposition(lval));
    } else {
      klgen_pushinst(gen, klinst_indexas(val.index, indexable.index, key.index), klgen_cstposition(lval));
    }
    klgen_stackfree(gen, stktop);
  } else if (klcst_kind(lval) == KLCST_EXPR_DOT) {
    size_t stktop = klgen_stacktop(gen);
    KlCstDot* dotcst = klcast(KlCstDot*, lval);
    KlCst* objcst = dotcst->operand;
    KlCodeVal obj = klgen_expr(gen, objcst);
    KlConstant constant = { .type = KL_STRING, .string = dotcst->field };
    KlConEntry* conent = klcontbl_get(gen->contbl, &constant);
    klgen_oomifnull(conent);
    KlCodeVal val = klgen_expr(gen, rval);
    klgen_putinstack(gen, &val, klgen_cstposition(rval));
    if (klinst_inurange(conent->index, 8)) {
      if (obj.kind == KLVAL_REF) {
        klgen_pushinst(gen, klinst_refsetfieldc(val.index, obj.index, conent->index), klgen_cstposition(lval));
      } else {
        klgen_putinstack(gen, &obj, klgen_cstposition(objcst));
        klgen_pushinst(gen, klinst_setfieldc(val.index, obj.index, conent->index), klgen_cstposition(lval));
      }
    } else {
      klgen_
      if (obj.kind == KLVAL_REF) {
        klgen_pushinst(gen, klinst_refsetfieldr(val.index, obj.index, conent->index), klgen_cstposition(lval));
      } else {
        klgen_putinstack(gen, &obj, klgen_cstposition(objcst));
        klgen_pushinst(gen, klinst_setfieldc(val.index, obj.index, conent->index), klgen_cstposition(lval));
      }
    }
    KlCodeVal rval_val = klgen_expr(gen, rval);
    klgen_putinstack(gen, &rval_val, klgen_cstposition(rval));
    if (index_val.kind == KLVAL_INTEGER) {
      klgen_pushinst(gen, klinst_indexasi(rval_val.index, indexable_val.index, index_val.intval), klgen_cstposition(lval));
    } else {
      klgen_pushinst(gen, klinst_indexas(rval_val.index, indexable_val.index, index_val.index), klgen_cstposition(lval));
    }
    klgen_stackfree(gen, stktop);
  } else {
    klgen_error(gen, klcst_begin(lval), klcst_end(lval), "can not be evaluated to an lvalue");
  }
}

static void klgen_stmtassign(KlGenUnit* gen, KlCstStmtAssign* assigncst) {
  KlCstTuple* lvals = klcast(KlCstTuple*, assigncst->lvals);
  KlCstTuple* rvals = klcast(KlCstTuple*, assigncst->rvals);
  kl_assert(lvals->nelem != 0, "");
  kl_assert(rvals->nelem != 0, "");
  klgen_tuple(gen, rvals, rvals->nelem - 1);
  if (lvals->nelem > rvals->nelem) {
    klgen_multival(gen, rvals->elems[rvals->nelem - 1], lvals->nelem - rvals->nelem + 1);

  } else if (lvals->nelem < rvals->nelem) {
    klgen_expr(gen, rvals->elems[rvals->nelem - 1]);  /* discard last value */
  }
}
