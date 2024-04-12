#include "klang/include/code/klgen_stmt.h"
#include "klang/include/code/klcodeval.h"
#include "klang/include/code/klcontbl.h"
#include "klang/include/code/klgen.h"
#include "klang/include/code/klgen_expr.h"
#include "klang/include/code/klgen_exprbool.h"
#include "klang/include/code/klgen_pattern.h"
#include "klang/include/code/klsymtbl.h"
#include "klang/include/cst/klcst.h"
#include "klang/include/cst/klcst_expr.h"
#include "klang/include/cst/klcst_stmt.h"
#include <unistd.h>


bool klgen_stmtblock(KlGenUnit *gen, KlCstStmtList *stmtlist) {
  klgen_pushsymtbl(gen);
  klgen_stmtlist(gen, stmtlist);
  bool needclose = gen->symtbl->info.referenced;
  klgen_popsymtbl(gen);
  return needclose;
}

void klgen_stmtlistpure(KlGenUnit* gen, KlCstStmtList* stmtlist) {
  KlCodeVal* prev_bjmp = gen->info.breakjmp;
  KlCodeVal* prev_cjmp = gen->info.continuejmp;
  KlSymTbl* prev_bscope = gen->info.break_scope;
  KlSymTbl* prev_cscope = gen->info.continue_scope;
  gen->info.breakjmp = NULL;
  gen->info.continuejmp = NULL;
  gen->info.break_scope = NULL;
  gen->info.continue_scope = NULL;
  klgen_stmtlist(gen, stmtlist);
  gen->info.breakjmp = prev_bjmp;
  gen->info.continuejmp = prev_cjmp;
  gen->info.break_scope = prev_bscope;
  gen->info.continue_scope = prev_cscope;
}

bool klgen_stmtblockpure(KlGenUnit* gen, KlCstStmtList* stmtlist) {
  KlCodeVal* prev_bjmp = gen->info.breakjmp;
  KlCodeVal* prev_cjmp = gen->info.continuejmp;
  KlSymTbl* prev_bscope = gen->info.break_scope;
  KlSymTbl* prev_cscope = gen->info.continue_scope;
  gen->info.breakjmp = NULL;
  gen->info.continuejmp = NULL;
  gen->info.break_scope = NULL;
  gen->info.continue_scope = NULL;
  bool needclose = klgen_stmtblock(gen, stmtlist);
  gen->info.breakjmp = prev_bjmp;
  gen->info.continuejmp = prev_cjmp;
  gen->info.break_scope = prev_bscope;
  gen->info.continue_scope = prev_cscope;
  return needclose;
}

static void klgen_stmtlet(KlGenUnit* gen, KlCstStmtLet* letcst) {
  size_t oristktop = klgen_stacktop(gen);
  KlCstTuple* rvals = klcast(KlCstTuple*, letcst->rvals);
  KlCst** patterns = klcast(KlCstTuple*, letcst->lvals)->elems;
  size_t npattern = klcast(KlCstTuple*, letcst->lvals)->nelem;
  size_t nrval = rvals->nelem;
  /* do fast assignment */
  size_t nfastassign = 0;
  size_t fastassignlimit = npattern < nrval ? npattern : nrval;
  for (; nfastassign < fastassignlimit; ++nfastassign) {
    KlCst* lval = klgen_exprpromotion(patterns[nfastassign]);
    if (klcst_kind(lval) != KLCST_EXPR_ID)
      break;
    klgen_exprtarget_noconst(gen, rvals->elems[nfastassign], klgen_stacktop(gen));
  }
  size_t nvar = klgen_patterns_assign_stkid(gen, patterns, npattern, klgen_stacktop(gen));
  klgen_stackalloc(gen, nvar);
  klgen_exprlist_raw(gen, rvals->elems + nfastassign, nrval - nfastassign, npattern - nfastassign, klgen_cstposition(rvals));
  kl_assert(klgen_stacktop(gen) == oristktop + nvar + npattern, "");
  size_t targettail = oristktop + nfastassign + nvar - 1;
  size_t count = npattern;
  while (count-- >= nfastassign) {
    targettail -= klgen_pattern_extract(gen, patterns[count], targettail);
  }
  for (size_t i = 0; i < npattern; ++i)
    klgen_pattern_newsymbol(gen, patterns[i]);
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
      klgen_oomifnull(gen, conent);
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
    klgen_oomifnull(gen, conent);
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
      klgen_oomifnull(gen, conent);
      klgen_pushinst(gen, klinst_storeglobal(stkid, conent->index), klgen_cstposition(lval));
    } else if (symbol->attr.kind == KLVAL_REF) {
      klgen_pushinst(gen, klinst_storeref(stkid, symbol->attr.idx), klgen_cstposition(lval));
    } else {
      kl_assert(symbol->attr.kind == KLVAL_STACK, "");
      kl_assert(symbol->attr.idx < klgen_stacktop(gen), "");
      kl_assert(symbol->attr.idx != stkid, "");
      klgen_movevals(gen, symbol->attr.idx, stkid, 1, klgen_cstposition(lval));
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
    klgen_oomifnull(gen, conent);
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
    klgen_multival(gen, rvals->elems[rvals->nelem - 1], lvals->nelem - rvals->nelem + 1, base);
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
      bool needclose = klgen_stmtblock(gen, klcast(KlCstStmtList*, ifcst->if_block));
      if (needclose)
        klgen_pushinst(gen, klinst_close(stktop), klgen_cstposition(ifcst->if_block));
      return;
    } else {
      if (!ifcst->else_block) return;
      size_t stktop = klgen_stacktop(gen);
      bool needclose = klgen_stmtblock(gen, klcast(KlCstStmtList*, ifcst->else_block));
      if (needclose)
        klgen_pushinst(gen, klinst_close(stktop), klgen_cstposition(ifcst->else_block));
      return;
    }
  } else {
    size_t stktop = klgen_stacktop(gen);
    bool ifneedclose = klgen_stmtblock(gen, klcast(KlCstStmtList*, ifcst->if_block));
    if (!ifcst->else_block) {
      if (ifneedclose)
        klgen_pushinst(gen, klinst_close(stktop), klgen_cstposition(ifcst->if_block));
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
      klgen_pushinst(gen, klinst_close(stktop), klgen_cstposition(ifcst->if_block));
    klgen_setinstjmppos(gen, ifout, klgen_currcodesize(gen));
  }
}

static void klgen_stmtwhile(KlGenUnit* gen, KlCstStmtWhile* whilecst) {
  KlCodeVal bjmplist = klcodeval_none();
  KlCodeVal cjmplist = klcodeval_jmp(klgen_pushinst(gen, klinst_jmp(0), klgen_cstposition(whilecst)));
  KlCodeVal* prev_bjmp = gen->info.breakjmp;
  KlCodeVal* prev_cjmp = gen->info.continuejmp;
  KlSymTbl* prev_bscope = gen->info.break_scope;
  KlSymTbl* prev_cscope = gen->info.continue_scope;
  gen->info.breakjmp = &bjmplist;
  gen->info.continuejmp = &cjmplist;
  gen->info.break_scope = gen->symtbl;
  gen->info.continue_scope = gen->symtbl;

  size_t stktop = klgen_stacktop(gen);
  size_t loopbeginpos = klgen_currcodesize(gen);
  bool needclose = klgen_stmtblock(gen, klcast(KlCstStmtList*, whilecst->block));
  if (needclose)
      klgen_pushinst(gen, klinst_close(stktop), klgen_cstposition(whilecst));
  size_t continuepos = klgen_currcodesize(gen);
  KlCodeVal cond = klgen_exprbool(gen, whilecst->cond, true);
  if (klcodeval_isconstant(cond)) {
    if (klcodeval_istrue(cond)) {
      bool cond_has_side_effect = klgen_currcodesize(gen) != continuepos;
      int offset = (int)loopbeginpos - (int)klgen_currcodesize(gen) - 1;
      if (!klinst_inrange(offset, 24))
        klgen_error_fatal(gen, "jump too far, can not generate code");
      klgen_pushinst(gen, klinst_jmp(offset), klgen_cstposition(whilecst->cond));
      klgen_setinstjmppos(gen, cjmplist, cond_has_side_effect ? continuepos : loopbeginpos);
      klgen_setinstjmppos(gen, bjmplist, klgen_currcodesize(gen));
    } else {
      klgen_setinstjmppos(gen, cjmplist, continuepos);
      klgen_setinstjmppos(gen, bjmplist, klgen_currcodesize(gen));
    }
  } else {
    klgen_setinstjmppos(gen, cond, loopbeginpos);
    klgen_setinstjmppos(gen, cjmplist, continuepos);
    klgen_setinstjmppos(gen, bjmplist, klgen_currcodesize(gen));
  }
  gen->info.breakjmp = prev_bjmp;
  gen->info.continuejmp = prev_cjmp;
  gen->info.break_scope = prev_bscope;
  gen->info.continue_scope = prev_cscope;
}

static void klgen_stmtrepeat(KlGenUnit* gen, KlCstStmtRepeat* repeatcst) {
  KlCodeVal bjmplist = klcodeval_none();
  KlCodeVal cjmplist = klcodeval_none();
  KlCodeVal* prev_bjmp = gen->info.breakjmp;
  KlCodeVal* prev_cjmp = gen->info.continuejmp;
  KlSymTbl* prev_bscope = gen->info.break_scope;
  KlSymTbl* prev_cscope = gen->info.continue_scope;
  gen->info.breakjmp = &bjmplist;
  gen->info.continuejmp = &cjmplist;
  gen->info.break_scope = gen->symtbl;
  gen->info.continue_scope = gen->symtbl;

  size_t stktop = klgen_stacktop(gen);
  size_t loopbeginpos = klgen_currcodesize(gen);
  bool needclose = klgen_stmtblock(gen, klcast(KlCstStmtList*, repeatcst->block));
  if (needclose)
      klgen_pushinst(gen, klinst_close(stktop), klgen_cstposition(repeatcst));
  size_t continuepos = klgen_currcodesize(gen);
  KlCodeVal cond = klgen_exprbool(gen, repeatcst->cond, true);
  if (klcodeval_isconstant(cond)) {
    if (klcodeval_istrue(cond)) {
      klgen_setinstjmppos(gen, cjmplist, continuepos);
      klgen_setinstjmppos(gen, bjmplist, klgen_currcodesize(gen));
    } else {
      bool cond_has_side_effect = klgen_currcodesize(gen) != continuepos;
      int offset = (int)loopbeginpos - (int)klgen_currcodesize(gen) - 1;
      if (!klinst_inrange(offset, 24))
        klgen_error_fatal(gen, "jump too far, can not generate code");
      klgen_pushinst(gen, klinst_jmp(offset), klgen_cstposition(repeatcst->cond));
      klgen_setinstjmppos(gen, cjmplist, cond_has_side_effect ? continuepos : loopbeginpos);
      klgen_setinstjmppos(gen, bjmplist, klgen_currcodesize(gen));
    }
  } else {
    klgen_setinstjmppos(gen, cond, loopbeginpos);
    klgen_setinstjmppos(gen, cjmplist, continuepos);
    klgen_setinstjmppos(gen, bjmplist, klgen_currcodesize(gen));
  }
  gen->info.breakjmp = prev_bjmp;
  gen->info.continuejmp = prev_cjmp;
  gen->info.break_scope = prev_bscope;
  gen->info.continue_scope = prev_cscope;
}

static bool klgen_needclose(KlGenUnit* gen, KlSymTbl* endtbl, size_t* closebase) {
  KlSymTbl* symtbl = gen->symtbl;
  bool needclose = false;
  while (symtbl != endtbl) {
    if (symtbl->info.referenced) {
      needclose = true;
      if (closebase) *closebase = symtbl->info.stkbase;
    }
    symtbl = klsymtbl_parent(symtbl);
  }
  return needclose;
}

static void klgen_stmtbreak(KlGenUnit* gen, KlCstStmtBreak* breakcst) {
  if (kl_unlikely(!gen->info.breakjmp)) {
    klgen_error(gen, klcst_begin(breakcst), klcst_end(breakcst), "break statement is not allowed here");
    return;
  }
  size_t closebase;
  KlInstruction breakinst;
  if (klgen_needclose(gen, gen->info.break_scope, &closebase)) {
    breakinst = klinst_closejmp(closebase, 0);
  } else {
    breakinst = klinst_jmp(0);
  }
  klgen_mergejmp_maynone(gen, gen->info.breakjmp,
                         klcodeval_jmp(klgen_pushinst(gen, breakinst, klgen_cstposition(breakcst))));
}

static void klgen_stmtcontinue(KlGenUnit* gen, KlCstStmtContinue* continuecst) {
  if (kl_unlikely(!gen->info.continuejmp)) {
    klgen_error(gen, klcst_begin(continuecst), klcst_end(continuecst), "continue statement is not allowed here");
    return;
  }
  size_t closebase;
  KlInstruction continueinst;
  if (klgen_needclose(gen, gen->info.continue_scope, &closebase)) {
    continueinst = klinst_closejmp(closebase, 0);
  } else {
    continueinst = klinst_jmp(0);
  }
  klgen_mergejmp_maynone(gen, gen->info.continuejmp,
                         klcodeval_jmp(klgen_pushinst(gen, continueinst, klgen_cstposition(continuecst))));
}

static void klgen_stmtreturn(KlGenUnit* gen, KlCstStmtReturn* returncst) {
  KlCstTuple* res = klcast(KlCstTuple*, returncst->retval);
  bool needclose = klgen_needclose(gen, gen->reftbl, NULL);
  if (res->nelem == 0) {
    klgen_expr(gen, klcst(res));
    if (needclose)
      klgen_pushinst(gen, klinst_close(0), klgen_cstposition(returncst));
    klgen_pushinst(gen, klinst_return0(), klgen_cstposition(returncst));
  } else if (res->nelem == 1) {
    KlCodeVal retval = klgen_expr(gen, klcst(res));
    klgen_putinstack(gen, &retval, klgen_cstposition(res));
    if (needclose)
      klgen_pushinst(gen, klinst_close(0), klgen_cstposition(returncst));
    klgen_pushinst(gen, klinst_return1(retval.index), klgen_cstposition(returncst));
  } else {
    size_t argbase = klgen_stacktop(gen);
    size_t nres = klgen_passargs(gen, klcst(res));
    if (needclose)
      klgen_pushinst(gen, klinst_close(0), klgen_cstposition(returncst));
    klgen_pushinst(gen, klinst_return(argbase, nres), klgen_cstposition(returncst));
  }
}

static void klgen_stmtifor(KlGenUnit* gen, KlCstStmtIFor* iforcst) {
  KlCodeVal bjmplist = klcodeval_none();
  KlCodeVal cjmplist = klcodeval_none();
  KlCodeVal* prev_bjmp = gen->info.breakjmp;
  KlCodeVal* prev_cjmp = gen->info.continuejmp;
  KlSymTbl* prev_bscope = gen->info.break_scope;
  KlSymTbl* prev_cscope = gen->info.continue_scope;
  gen->info.breakjmp = &bjmplist;
  gen->info.continuejmp = &cjmplist;
  gen->info.break_scope = gen->symtbl;
  gen->info.continue_scope = gen->symtbl;

  size_t forbase = klgen_stacktop(gen);
  KlCodeVal begin = klgen_exprtarget(gen, iforcst->begin, forbase);
  if (klcodeval_isconstant(begin))
    klgen_putinstktop(gen, &begin, klgen_cstposition(iforcst->begin));
  KlCodeVal end = klgen_exprtarget(gen, iforcst->end, forbase + 1);
  if (klcodeval_isconstant(end))
    klgen_putinstktop(gen, &end, klgen_cstposition(iforcst->step));
  KlCodeVal step;
  if (iforcst->step) {
    step = klgen_exprtarget(gen, iforcst->step, forbase + 2);
    if (klcodeval_isconstant(step))
      klgen_putinstktop(gen, &step, klgen_cstposition(iforcst->step));
  } else {
    klgen_loadnils(gen, forbase + 2, 1, klgen_position(klcst_end(iforcst->end), klcst_end(iforcst->end)));
    step = klcodeval_stack(forbase + 2);
  }
  kl_assert(begin.index == forbase && end.index == forbase + 1 && step.index == forbase + 2, "");
  kl_assert(klgen_stacktop(gen) == forbase + 3, "");

  klgen_mergejmp_maynone(gen, gen->info.breakjmp,
                         klcodeval_jmp(klgen_pushinst(gen, klinst_iforprep(forbase, 0), klgen_cstposition(iforcst))));
  klgen_pushsymtbl(gen);  /* begin a new scope here */
  klgen_newsymbol(gen, iforcst->id, forbase, klcst_begin(iforcst)); /* create iteration variable */
  size_t looppos = klgen_currcodesize(gen);
  klgen_stmtlist(gen, klcast(KlCstStmtList*, iforcst->block));
  if (gen->symtbl->info.referenced)
      klgen_pushinst(gen, klinst_close(forbase), klgen_cstposition(iforcst));
  klgen_popsymtbl(gen);   /* close the scope */
  klgen_setinstjmppos(gen, cjmplist, klgen_currcodesize(gen));
  klgen_pushinst(gen, klinst_iforloop(forbase, looppos - klgen_currcodesize(gen) - 1), klgen_cstposition(iforcst));
  klgen_setinstjmppos(gen, bjmplist, klgen_currcodesize(gen));
  klgen_stackfree(gen, forbase);

  gen->info.breakjmp = prev_bjmp;
  gen->info.continuejmp = prev_cjmp;
  gen->info.break_scope = prev_bscope;
  gen->info.continue_scope = prev_cscope;
}

static void klgen_stmtvfor(KlGenUnit* gen, KlCstStmtVFor* vforcst) {
  KlCodeVal bjmplist = klcodeval_none();
  KlCodeVal cjmplist = klcodeval_none();
  KlCodeVal* prev_bjmp = gen->info.breakjmp;
  KlCodeVal* prev_cjmp = gen->info.continuejmp;
  KlSymTbl* prev_bscope = gen->info.break_scope;
  KlSymTbl* prev_cscope = gen->info.continue_scope;
  gen->info.breakjmp = &bjmplist;
  gen->info.continuejmp = &cjmplist;
  gen->info.break_scope = gen->symtbl;
  gen->info.continue_scope = gen->symtbl;

  size_t forbase = klgen_stacktop(gen);
  KlCodeVal step = klcodeval_integer(vforcst->nid);
  klgen_putinstktop(gen, &step, klgen_cstposition(vforcst));
  klgen_mergejmp_maynone(gen, gen->info.breakjmp,
                         klcodeval_jmp(klgen_pushinst(gen, klinst_vforprep(forbase, 0), klgen_cstposition(vforcst))));
  klgen_stackalloc1(gen); /* forbase + 1: step, forbase + 2: index */
  klgen_pushsymtbl(gen);  /* begin a new scope */
  size_t nid = vforcst->nid;
  KlStrDesc* ids = vforcst->ids;
  klgen_stackalloc(gen, nid);
  for (size_t i = 0; i < nid; ++i)
    klgen_newsymbol(gen, ids[i], forbase + 2 + i, klcst_begin(vforcst));
  klgen_mergejmp_maynone(gen, gen->info.breakjmp,
                         klcodeval_jmp(klgen_pushinst(gen, klinst_vforprep(forbase, 0), klgen_cstposition(vforcst))));
  size_t looppos = klgen_currcodesize(gen);
  klgen_stmtlist(gen, klcast(KlCstStmtList*, vforcst->block));
  if (gen->symtbl->info.referenced)
      klgen_pushinst(gen, klinst_close(forbase), klgen_cstposition(vforcst));
  klgen_popsymtbl(gen);   /* close the scope */
  klgen_setinstjmppos(gen, cjmplist, klgen_currcodesize(gen));
  klgen_pushinst(gen, klinst_vforloop(forbase, looppos - klgen_currcodesize(gen) - 1), klgen_cstposition(vforcst));
  klgen_setinstjmppos(gen, bjmplist, klgen_currcodesize(gen));
  klgen_stackfree(gen, forbase);

  gen->info.breakjmp = prev_bjmp;
  gen->info.continuejmp = prev_cjmp;
  gen->info.break_scope = prev_bscope;
  gen->info.continue_scope = prev_cscope;
}

static void klgen_stmtgfor(KlGenUnit* gen, KlCstStmtGFor* gforcst) {
  KlCodeVal bjmplist = klcodeval_none();
  KlCodeVal cjmplist = klcodeval_none();
  KlCodeVal* prev_bjmp = gen->info.breakjmp;
  KlCodeVal* prev_cjmp = gen->info.continuejmp;
  KlSymTbl* prev_bscope = gen->info.break_scope;
  KlSymTbl* prev_cscope = gen->info.continue_scope;
  gen->info.breakjmp = &bjmplist;
  gen->info.continuejmp = &cjmplist;
  gen->info.break_scope = gen->symtbl;
  gen->info.continue_scope = gen->symtbl;

  size_t forbase = klgen_stacktop(gen);
  KlCodeVal iterable = klgen_exprtarget(gen, gforcst->expr, forbase);
  if (klcodeval_isconstant(iterable))
    klgen_putinstktop(gen, &iterable, klgen_cstposition(gforcst->expr));
  kl_assert(iterable.index == forbase, "");
  klgen_stackfree(gen, forbase);
  KlConstant constant = { .type = KL_STRING, .string = gen->string.itermethod };
  KlConEntry* conent = klcontbl_get(gen->contbl, &constant);
  klgen_oomifnull(gen, conent);
  klgen_pushinstmethod(gen, iterable.index, conent->index, 0, 3, forbase, klgen_cstposition(gforcst));
  klgen_mergejmp_maynone(gen, gen->info.continuejmp,
                         klcodeval_jmp(klgen_pushinst(gen, klinst_jmp(0), klgen_cstposition(gforcst))));
  klgen_pushsymtbl(gen);    /* begin a new scope */
  klgen_stackalloc(gen, 3); /* forbase: iteration function, forbase + 1: static state, forbase + 2: index state */
  size_t nid = gforcst->nid;
  KlStrDesc* ids = gforcst->ids;
  klgen_stackalloc(gen, nid);
  for (size_t i = 0; i < nid; ++i)
    klgen_newsymbol(gen, ids[i], forbase + 3 + i, klcst_begin(gforcst));
  size_t looppos = klgen_currcodesize(gen);
  klgen_stmtlist(gen, klcast(KlCstStmtList*, gforcst->block));
  if (gen->symtbl->info.referenced)
      klgen_pushinst(gen, klinst_close(forbase), klgen_cstposition(gforcst));
  klgen_popsymtbl(gen);   /* close the scope */
  klgen_setinstjmppos(gen, cjmplist, klgen_currcodesize(gen));
  klgen_pushinst(gen, klinst_gforloop(forbase, nid + 2), klgen_cstposition(gforcst));
  klgen_pushinst(gen, klinst_truejmp(forbase + 1, looppos - klgen_currcodesize(gen) - 1), klgen_cstposition(gforcst));
  klgen_setinstjmppos(gen, bjmplist, klgen_currcodesize(gen));
  klgen_stackfree(gen, forbase);

  gen->info.breakjmp = prev_bjmp;
  gen->info.continuejmp = prev_cjmp;
  gen->info.break_scope = prev_bscope;
  gen->info.continue_scope = prev_cscope;
}

void klgen_stmtlist(KlGenUnit* gen, KlCstStmtList* cst) {
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
        klgen_multival(gen, klcast(KlCstStmtExpr*, stmt)->expr, 0, stktop);
        kl_assert(klgen_stacktop(gen) == stktop, "");
        break;
      }
      case KLCST_STMT_BLOCK: {
        klgen_stmtblock(gen, klcast(KlCstStmtList*, stmt));
        break;
      }
      case KLCST_STMT_IF: {
        klgen_stmtif(gen, klcast(KlCstStmtIf*, stmt));
        break;
      }
      case KLCST_STMT_IFOR: {
        klgen_stmtifor(gen, klcast(KlCstStmtIFor*, stmt));
        break;
      }
      case KLCST_STMT_VFOR: {
        klgen_stmtvfor(gen, klcast(KlCstStmtVFor*, stmt));
        break;
      }
      case KLCST_STMT_GFOR: {
        klgen_stmtgfor(gen, klcast(KlCstStmtGFor*, stmt));
        break;
      }
      case KLCST_STMT_WHILE: {
        klgen_stmtwhile(gen, klcast(KlCstStmtWhile*, stmt));
        break;
      }
      case KLCST_STMT_REPEAT: {
        klgen_stmtrepeat(gen, klcast(KlCstStmtRepeat*, stmt));
        break;
      }
      case KLCST_STMT_BREAK: {
        klgen_stmtbreak(gen, klcast(KlCstStmtBreak*, stmt));
        break;
      }
      case KLCST_STMT_CONTINUE: {
        klgen_stmtcontinue(gen, klcast(KlCstStmtContinue*, stmt));
        break;
      }
      case KLCST_STMT_RETURN: {
        klgen_stmtreturn(gen, klcast(KlCstStmtReturn*, stmt));
        break;
      }
      default: {
        kl_assert(false, "control flow should not reach here");
        break;
      }
    }
  }
}
