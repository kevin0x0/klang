#include "klang/include/code/klgen_stmt.h"
#include "klang/include/code/klcodeval.h"
#include "klang/include/code/klcontbl.h"
#include "klang/include/code/klgen.h"
#include "klang/include/code/klgen_expr.h"
#include "klang/include/code/klgen_exprbool.h"
#include "klang/include/cst/klcst.h"
#include "klang/include/cst/klcst_expr.h"
#include "klang/include/cst/klcst_stmt.h"

static void klgen_stmtlet(KlGenUnit* gen, KlCstStmtLet* letcst) {
  size_t stkid = klgen_stacktop(gen);
  KlCstTuple* rvals = klcast(KlCstTuple*, letcst->rvals);
  klgen_tuple(gen, rvals, letcst->nlval);
  if (letcst->nlval < rvals->nelem) { /* tuple is not completely evaluated */
    size_t stktop = klgen_stacktop(gen);
    for (size_t i = letcst->nlval; i < rvals->nelem; ++i) {
      klgen_expr(gen, rvals->elems[i]);
      klgen_stackfree(gen, stktop);
    }
  }
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
      kl_assert(symbol->attr.idx < klgen_stacktop(gen), "");
      KlCodeVal res = klgen_exprtarget(gen, rval, symbol->attr.idx);
      if (klcodeval_isconstant(res))
        klgen_loadval(gen, symbol->attr.idx, res, klgen_cstposition(lval));
    }
  } else if (klcst_kind(lval) == KLCST_EXPR_POST && klcast(KlCstPost*, lval)->op == KLTK_INDEX) {
    size_t stktop = klgen_stacktop(gen);
    KlCstPost* indexcst = klcast(KlCstPost*, lval);
    KlCst* indexablecst = indexcst->operand;
    KlCst* keycst = indexcst->post;
    KlCodeVal val = klgen_expr(gen, rval);
    klgen_putinstack(gen, &val, klgen_cstposition(rval));
    KlCodeVal indexable = klgen_expr(gen, indexablecst);
    klgen_putinstack(gen, &indexable, klgen_cstposition(indexablecst));
    KlCodeVal key = klgen_expr(gen, keycst);
    if (key.kind != KLVAL_INTEGER || !klinst_inrange(key.intval, 8))
      klgen_putinstack(gen, &key, klgen_cstposition(keycst));
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
    KlCodeVal val = klgen_expr(gen, rval);
    klgen_putinstack(gen, &val, klgen_cstposition(rval));
    KlCodeVal obj = klgen_expr(gen, objcst);
    KlConstant constant = { .type = KL_STRING, .string = dotcst->field };
    KlConEntry* conent = klcontbl_get(gen->contbl, &constant);
    klgen_oomifnull(conent);
    if (klinst_inurange(conent->index, 8)) {
      if (obj.kind == KLVAL_REF) {
        klgen_pushinst(gen, klinst_refsetfieldc(val.index, obj.index, conent->index), klgen_cstposition(lval));
      } else {
        klgen_putinstack(gen, &obj, klgen_cstposition(objcst));
        klgen_pushinst(gen, klinst_setfieldc(val.index, obj.index, conent->index), klgen_cstposition(lval));
      }
    } else {
      size_t tmp = klgen_stackalloc1(gen);
      klgen_pushinst(gen, klinst_loadc(tmp, conent->index), klgen_cstposition(lval));
      if (obj.kind == KLVAL_REF) {
        klgen_pushinst(gen, klinst_refsetfieldr(val.index, obj.index, tmp), klgen_cstposition(lval));
      } else {
        klgen_putinstack(gen, &obj, klgen_cstposition(objcst));
        klgen_pushinst(gen, klinst_setfieldr(val.index, obj.index, tmp), klgen_cstposition(lval));
      }
    }
    klgen_stackfree(gen, stktop);
  } else {
    klgen_error(gen, klcst_begin(lval), klcst_end(lval), "can not be evaluated to an lvalue");
  }
}

static void klgen_assignfrom(KlGenUnit* gen, KlCst* lval, size_t stkid) {
  if (klcst_kind(lval) == KLCST_EXPR_ID) {
    KlCstIdentifier* id = klcast(KlCstIdentifier*, lval);
    KlSymbol* symbol = klgen_getsymbol(gen, id->id);
    if (!symbol) {  /* global variable */
      KlConstant constant = { .type = KL_STRING, .string = id->id };
      KlConEntry* conent = klcontbl_get(gen->contbl, &constant);
      klgen_oomifnull(conent);
      klgen_pushinst(gen, klinst_storeglobal(stkid, conent->index), klgen_cstposition(lval));
    } else if (symbol->attr.kind == KLVAL_REF) {
      klgen_pushinst(gen, klinst_storeref(stkid, symbol->attr.idx), klgen_cstposition(lval));
    } else {
      kl_assert(symbol->attr.kind == KLVAL_STACK, "");
      kl_assert(symbol->attr.idx < klgen_stacktop(gen), "");
      kl_assert(symbol->attr.idx != stkid, "");
      klgen_pushinst(gen, klinst_move(symbol->attr.idx, stkid), klgen_cstposition(lval));
    }
  } else if (klcst_kind(lval) == KLCST_EXPR_POST && klcast(KlCstPost*, lval)->op == KLTK_INDEX) {
    size_t stktop = klgen_stacktop(gen);
    KlCstPost* indexcst = klcast(KlCstPost*, lval);
    KlCst* indexablecst = indexcst->operand;
    KlCst* keycst = indexcst->post;
    KlCodeVal indexable = klgen_expr(gen, indexablecst);
    klgen_putinstack(gen, &indexable, klgen_cstposition(indexablecst));
    KlCodeVal key = klgen_expr(gen, keycst);
    if (key.kind != KLVAL_INTEGER || !klinst_inrange(key.intval, 8))
      klgen_putinstack(gen, &key, klgen_cstposition(keycst));
    if (key.kind == KLVAL_INTEGER) {
      klgen_pushinst(gen, klinst_indexasi(stkid, indexable.index, key.intval), klgen_cstposition(lval));
    } else {
      klgen_pushinst(gen, klinst_indexas(stkid, indexable.index, key.index), klgen_cstposition(lval));
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
    if (klinst_inurange(conent->index, 8)) {
      if (obj.kind == KLVAL_REF) {
        klgen_pushinst(gen, klinst_refsetfieldc(stkid, obj.index, conent->index), klgen_cstposition(lval));
      } else {
        klgen_putinstack(gen, &obj, klgen_cstposition(objcst));
        klgen_pushinst(gen, klinst_setfieldc(stkid, obj.index, conent->index), klgen_cstposition(lval));
      }
    } else {
      size_t tmp = klgen_stackalloc1(gen);
      klgen_pushinst(gen, klinst_loadc(tmp, conent->index), klgen_cstposition(lval));
      if (obj.kind == KLVAL_REF) {
        klgen_pushinst(gen, klinst_refsetfieldr(stkid, obj.index, tmp), klgen_cstposition(lval));
      } else {
        klgen_putinstack(gen, &obj, klgen_cstposition(objcst));
        klgen_pushinst(gen, klinst_setfieldr(stkid, obj.index, tmp), klgen_cstposition(lval));
      }
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
  size_t base = klgen_stacktop(gen);
  klgen_tuple(gen, rvals, rvals->nelem - 1);
  if (lvals->nelem > rvals->nelem) {
    klgen_multival(gen, rvals->elems[rvals->nelem - 1], lvals->nelem - rvals->nelem + 1);
    for (size_t i = lvals->nelem; i-- > 0;)
      klgen_assignfrom(gen, lvals->elems[i], base + i);
  } else if (lvals->nelem < rvals->nelem) {
    klgen_expr(gen, rvals->elems[rvals->nelem - 1]);  /* discard last value */
    klgen_stackfree(gen, base + lvals->nelem);
    for (size_t i = lvals->nelem; i-- > 0;)
      klgen_assignfrom(gen, lvals->elems[i], base + i);
  } else {
    klgen_singleassign(gen, lvals->elems[lvals->nelem - 1], rvals->elems[rvals->nelem - 1]);
    for (size_t i = lvals->nelem - 1; i-- > 0;)
      klgen_assignfrom(gen, lvals->elems[i], base + i);
  }
}

static void klgen_stmtif(KlGenUnit* gen, KlCstStmtIf* ifcst) {
  KlCodeVal cond = klgen_exprbool(gen, ifcst->cond, false);
  if (klcodeval_isconstant(cond)) {
    if (klcodeval_istrue(cond)) {
      size_t stktop = klgen_stacktop(gen);
      bool ifneedclose = klgen_stmtblock(gen, klcast(KlCstStmtList*, ifcst->if_block));
      if (!ifcst->else_block) {
        if (ifneedclose)
          klgen_pushinst(gen, klinst_closejmp(stktop, 0), klgen_cstposition(ifcst->if_block));
        klgen_setinstjmppos(gen, cond, klgen_currcodesize(gen));
        return;
      }
    }
  } else {
    size_t stktop = klgen_stacktop(gen);
    bool ifneedclose = klgen_stmtblock(gen, klcast(KlCstStmtList*, ifcst->if_block));
    if (!ifcst->else_block) {
      if (ifneedclose)
        klgen_pushinst(gen, klinst_closejmp(stktop, 0), klgen_cstposition(ifcst->if_block));
      klgen_setinstjmppos(gen, cond, klgen_currcodesize(gen));
      return;
    }
    KlCodeVal ifout = klcodeval_jmp(klgen_pushinst(gen,
                                                   ifneedclose ? klinst_closejmp(stktop, 0)
                                                             : klinst_jmp(0),
                                                   klgen_cstposition(ifcst->if_block)));
    klgen_setinstjmppos(gen, cond, klgen_currcodesize(gen));
    bool elseneedclose = klgen_stmtblock(gen, klcast(KlCstStmtList*, ifcst->else_block));
    if (elseneedclose)
      klgen_pushinst(gen, klinst_closejmp(stktop, 0), klgen_cstposition(ifcst->if_block));
    klgen_setinstjmppos(gen, ifout, klgen_currcodesize(gen));
  }
}

void klgen_stmtlist(KlGenUnit* gen, KlCstStmtList* cst) {
  kltodo("push new block info");
  KlCst** endstmt = cst->stmts + cst->nstmt;
  for (KlCst** pstmt = cst->stmts; pstmt != endstmt; ++pstmt) {
    KlCst* stmt = *pstmt;
    switch (klcst_kind(stmt)) {
      case KLCST_STMT_LET: {
        klgen_stmtlet(gen, klcast(KlCstStmtLet*, stmt));
        break;
      }
      case KLCST_STMT_ASSIGN: {
        klgen_stmtassign(gen, klcast(KlCstStmtAssign*, stmt));
        break;
      }
      case KLCST_STMT_EXPR: {
        size_t stktop = klgen_stacktop(gen);
        klgen_expr(gen, klcast(KlCstStmtExpr*, stmt)->expr);
        klgen_stackfree(gen, stktop);
        break;
      }
      case KLCST_STMT_BLOCK: {
        klgen_stmtblock(gen, klcast(KlCstStmtList*, stmt));
        break;
      }
      case KLCST_STMT_IF: {
        kltodo("implement if");
      }
      case KLCST_STMT_IFOR: {
        kltodo("implement ifor");
      }
      case KLCST_STMT_VFOR: {
        kltodo("implement vfor");
      }
      case KLCST_STMT_GFOR: {
        kltodo("implement gfor");
      }
      case KLCST_STMT_CFOR: {
        kltodo("implement cfor");
      }
      case KLCST_STMT_BREAK: {
        kltodo("implement break");
      }
      case KLCST_STMT_WHILE: {
        kltodo("implement while");
      }
      case KLCST_STMT_REPEAT: {
        kltodo("implement repeat");
      }
      case KLCST_STMT_CONTINUE: {
        kltodo("implement continue");
      }
      case KLCST_STMT_RETURN: {
        kltodo("implement return");
      }
    }
  }
  kltodo("check reference and generate close instruction");
  kltodo("pop block info");
}
