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
#include "klang/include/vm/klinst.h"


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

static void klgen_deconstruct_to_stktop(KlGenUnit* gen, KlCst** patterns, size_t npattern, KlCst** rvals, size_t nrval, KlFilePosition filepos) {
  size_t nfastassign = 0;
  for (; nfastassign < npattern; ++nfastassign) {
    if (klcst_kind(klgen_exprpromotion(patterns[nfastassign])) != KLCST_EXPR_ID)
      break;
  }
  if (nfastassign == npattern) {
    klgen_exprlist_raw(gen, rvals, nrval, nfastassign, filepos);
  } else {  /* now nfastassign < npattern */
    if (nfastassign == npattern - 1) {
      klgen_exprlist_raw(gen, rvals, nrval, nfastassign, filepos);
      if (klgen_pattern_fastdeconstruct(gen, patterns[nfastassign]))
        return;
      size_t nreserved = klgen_pattern_count_result(gen, patterns[nfastassign]);
      size_t lastval = klgen_stacktop(gen) - 1;
      klgen_emitmove(gen, lastval + nreserved, lastval, 1, filepos);
      klgen_stackalloc(gen, nreserved);
      klgen_pattern_deconstruct(gen, patterns[nfastassign], lastval);
    } else if (nfastassign <= nrval) {
      klgen_exprlist_raw(gen, rvals, nfastassign, nfastassign, filepos);
      size_t nreserved = klgen_patterns_count_result(gen, patterns + nfastassign, npattern - nfastassign);
      klgen_stackalloc(gen, nreserved);
      size_t target = klgen_stacktop(gen);
      klgen_exprlist_raw(gen, rvals + nfastassign, nrval - nfastassign, npattern - nfastassign, filepos);
      size_t count = npattern;
      while (count-- > nfastassign)
        target -= klgen_pattern_deconstruct(gen, patterns[count], target);
    } else {
      size_t oristktop = klgen_stacktop(gen);
      klgen_exprlist_raw(gen, rvals, nrval, npattern, filepos);
      size_t nreserved = klgen_patterns_count_result(gen, patterns + nfastassign, npattern - nfastassign);
      klgen_stackfree(gen, oristktop + nreserved + npattern);
      klgen_emitmove(gen, oristktop + nfastassign + nreserved, oristktop + nfastassign, npattern - nfastassign, filepos);
      size_t target = oristktop + nfastassign + nreserved;
      size_t count = npattern;
      while (count-- > nfastassign)
        target -= klgen_pattern_deconstruct(gen, patterns[count], target);
    }
  }
}

static void klgen_stmtlet(KlGenUnit* gen, KlCstStmtLet* letcst) {
  KlCstTuple* lvals = klcast(KlCstTuple*, letcst->lvals);
  KlCstTuple* rvals = klcast(KlCstTuple*, letcst->rvals);
  size_t newsymbol_base = klgen_stacktop(gen);
  klgen_deconstruct_to_stktop(gen, lvals->elems, lvals->nelem, rvals->elems, rvals->nelem, klgen_cstposition(rvals));
  klgen_patterns_newsymbol(gen, lvals->elems, lvals->nelem, newsymbol_base);
}

static void klgen_singleassign(KlGenUnit* gen, KlCst* lval, KlCst* rval) {
  if (klcst_kind(lval) == KLCST_EXPR_ID) {
    KlCstIdentifier* id = klcast(KlCstIdentifier*, lval);
    KlSymbol* symbol = klgen_getsymbol(gen, id->id);
    if (!symbol) {  /* global variable */
      size_t stktop = klgen_stacktop(gen);
      KlCodeVal res = klgen_expr(gen, rval);
      klgen_putonstack(gen, &res, klgen_cstposition(rval));
      KlConstant constant = { .type = KL_STRING, .string = id->id };
      KlConEntry* conent = klcontbl_get(gen->contbl, &constant);
      klgen_oomifnull(gen, conent);
      klgen_emit(gen, klinst_storeglobal(res.index, conent->index), klgen_cstposition(lval));
      klgen_stackfree(gen, stktop);
    } else if (symbol->attr.kind == KLVAL_REF) {
      size_t stktop = klgen_stacktop(gen);
      KlCodeVal res = klgen_expr(gen, rval);
      klgen_putonstack(gen, &res, klgen_cstposition(rval));
      klgen_emit(gen, klinst_storeref(res.index, symbol->attr.idx), klgen_cstposition(lval));
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
    klgen_putonstack(gen, &val, klgen_cstposition(rval));
    KlCodeVal indexable = klgen_expr(gen, indexablecst);
    klgen_putonstack(gen, &indexable, klgen_cstposition(indexablecst));
    KlCodeVal key = klgen_expr(gen, keycst);
    if (key.kind != KLVAL_INTEGER || !klinst_inrange(key.intval, 8))
      klgen_putonstack(gen, &key, klgen_cstposition(keycst));
    if (key.kind == KLVAL_INTEGER) {
      klgen_emit(gen, klinst_indexasi(val.index, indexable.index, key.intval), klgen_cstposition(lval));
    } else {
      klgen_emit(gen, klinst_indexas(val.index, indexable.index, key.index), klgen_cstposition(lval));
    }
    klgen_stackfree(gen, stktop);
  } else if (klcst_kind(lval) == KLCST_EXPR_DOT) {
    size_t stktop = klgen_stacktop(gen);
    KlCstDot* dotcst = klcast(KlCstDot*, lval);
    KlCst* objcst = dotcst->operand;
    KlCodeVal val = klgen_expr(gen, rval);
    klgen_putonstack(gen, &val, klgen_cstposition(rval));
    KlCodeVal obj = klgen_expr(gen, objcst);
    KlConstant constant = { .type = KL_STRING, .string = dotcst->field };
    KlConEntry* conent = klcontbl_get(gen->contbl, &constant);
    klgen_oomifnull(gen, conent);
    if (klinst_inurange(conent->index, 8)) {
      if (obj.kind == KLVAL_REF) {
        klgen_emit(gen, klinst_refsetfieldc(val.index, obj.index, conent->index), klgen_cstposition(lval));
      } else {
        klgen_putonstack(gen, &obj, klgen_cstposition(objcst));
        klgen_emit(gen, klinst_setfieldc(val.index, obj.index, conent->index), klgen_cstposition(lval));
      }
    } else {
      size_t tmp = klgen_stackalloc1(gen);
      klgen_emit(gen, klinst_loadc(tmp, conent->index), klgen_cstposition(lval));
      if (obj.kind == KLVAL_REF) {
        klgen_emit(gen, klinst_refsetfieldr(val.index, obj.index, tmp), klgen_cstposition(lval));
      } else {
        klgen_putonstack(gen, &obj, klgen_cstposition(objcst));
        klgen_emit(gen, klinst_setfieldr(val.index, obj.index, tmp), klgen_cstposition(lval));
      }
    }
    klgen_stackfree(gen, stktop);
  } else {
    klgen_error(gen, klcst_begin(lval), klcst_end(lval), "can not be evaluated to an lvalue");
  }
}

void klgen_assignfrom(KlGenUnit* gen, KlCst* lval, size_t stkid) {
  if (klcst_kind(lval) == KLCST_EXPR_ID) {
    KlCstIdentifier* id = klcast(KlCstIdentifier*, lval);
    KlSymbol* symbol = klgen_getsymbol(gen, id->id);
    if (!symbol) {  /* global variable */
      KlConstant constant = { .type = KL_STRING, .string = id->id };
      KlConEntry* conent = klcontbl_get(gen->contbl, &constant);
      klgen_oomifnull(gen, conent);
      klgen_emit(gen, klinst_storeglobal(stkid, conent->index), klgen_cstposition(lval));
    } else if (symbol->attr.kind == KLVAL_REF) {
      klgen_emit(gen, klinst_storeref(stkid, symbol->attr.idx), klgen_cstposition(lval));
    } else {
      kl_assert(symbol->attr.kind == KLVAL_STACK, "");
      kl_assert(symbol->attr.idx < klgen_stacktop(gen), "");
      kl_assert(symbol->attr.idx != stkid, "");
      klgen_emitmove(gen, symbol->attr.idx, stkid, 1, klgen_cstposition(lval));
    }
  } else if (klcst_kind(lval) == KLCST_EXPR_POST && klcast(KlCstPost*, lval)->op == KLTK_INDEX) {
    size_t stktop = klgen_stacktop(gen);
    KlCstPost* indexcst = klcast(KlCstPost*, lval);
    KlCst* indexablecst = indexcst->operand;
    KlCst* keycst = indexcst->post;
    KlCodeVal indexable = klgen_expr(gen, indexablecst);
    klgen_putonstack(gen, &indexable, klgen_cstposition(indexablecst));
    KlCodeVal key = klgen_expr(gen, keycst);
    if (key.kind != KLVAL_INTEGER || !klinst_inrange(key.intval, 8))
      klgen_putonstack(gen, &key, klgen_cstposition(keycst));
    if (key.kind == KLVAL_INTEGER) {
      klgen_emit(gen, klinst_indexasi(stkid, indexable.index, key.intval), klgen_cstposition(lval));
    } else {
      klgen_emit(gen, klinst_indexas(stkid, indexable.index, key.index), klgen_cstposition(lval));
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
        klgen_emit(gen, klinst_refsetfieldc(stkid, obj.index, conent->index), klgen_cstposition(lval));
      } else {
        klgen_putonstack(gen, &obj, klgen_cstposition(objcst));
        klgen_emit(gen, klinst_setfieldc(stkid, obj.index, conent->index), klgen_cstposition(lval));
      }
    } else {
      size_t tmp = klgen_stackalloc1(gen);
      klgen_emit(gen, klinst_loadc(tmp, conent->index), klgen_cstposition(lval));
      if (obj.kind == KLVAL_REF) {
        klgen_emit(gen, klinst_refsetfieldr(stkid, obj.index, tmp), klgen_cstposition(lval));
      } else {
        klgen_putonstack(gen, &obj, klgen_cstposition(objcst));
        klgen_emit(gen, klinst_setfieldr(stkid, obj.index, tmp), klgen_cstposition(lval));
      }
    }
    klgen_stackfree(gen, stktop);
  } else {
    klgen_error(gen, klcst_begin(lval), klcst_end(lval), "can not be evaluated to an lvalue");
  }
}

static inline bool klgen_canassign(KlCst* lval) {
  return (klcst_kind(lval) == KLCST_EXPR_ID   ||
          klcst_kind(lval) == KLCST_EXPR_DOT  ||
          (klcst_kind(lval) == KLCST_EXPR_POST &&
           klcast(KlCstPost*, lval)->op == KLTK_INDEX));
}

static void klgen_stmtassign(KlGenUnit* gen, KlCstStmtAssign* assigncst) {
  kltodo("support pattern deconstruction");
  KlCst** patterns = klcast(KlCstTuple*, assigncst->lvals)->elems;
  size_t npattern = klcast(KlCstTuple*, assigncst->lvals)->nelem;
  KlCst** rvals = klcast(KlCstTuple*, assigncst->rvals)->elems;
  size_t nrval = klcast(KlCstTuple*, assigncst->rvals)->nelem;
  kl_assert(npattern != 0, "");
  kl_assert(nrval != 0, "");
  size_t base = klgen_stacktop(gen);
  KlFilePosition rvals_pos = klgen_cstposition(assigncst->rvals);
  if (nrval != npattern) {
    klgen_deconstruct_to_stktop(gen, patterns, npattern, rvals, nrval, rvals_pos);
    klgen_patterns_do_assignment(gen, patterns, npattern);
    klgen_stackfree(gen, base);
  } else {
    for (size_t i = 0; i < npattern; ++i) {
      if (!klgen_canassign(klgen_exprpromotion(patterns[i]))) {
        klgen_deconstruct_to_stktop(gen, patterns, npattern - 1, rvals, nrval - 1, rvals_pos);
        klgen_patterns_do_assignment(gen, patterns, npattern);
        klgen_stackfree(gen, base);
        return;
      }
    }
    klgen_exprlist_raw(gen, rvals, nrval - 1, nrval - 1, rvals_pos);
    klgen_singleassign(gen, patterns[npattern - 1], rvals[nrval - 1]);
    for (size_t i = nrval - 1; i-- > 0;)
      klgen_assignfrom(gen, patterns[i], base + i);
    klgen_stackfree(gen, base);
  }
}

static void klgen_stmtif(KlGenUnit* gen, KlCstStmtIf* ifcst) {
  KlCodeVal cond = klgen_exprbool(gen, ifcst->cond, false);
  if (klcodeval_isconstant(cond)) {
    if (klcodeval_istrue(cond)) {
      size_t stktop = klgen_stacktop(gen);
      bool needclose = klgen_stmtblock(gen, klcast(KlCstStmtList*, ifcst->if_block));
      if (needclose)
        klgen_emit(gen, klinst_close(stktop), klgen_cstposition(ifcst->if_block));
      return;
    } else {
      if (!ifcst->else_block) return;
      size_t stktop = klgen_stacktop(gen);
      bool needclose = klgen_stmtblock(gen, klcast(KlCstStmtList*, ifcst->else_block));
      if (needclose)
        klgen_emit(gen, klinst_close(stktop), klgen_cstposition(ifcst->else_block));
      return;
    }
  } else {
    size_t stktop = klgen_stacktop(gen);
    bool ifneedclose = klgen_stmtblock(gen, klcast(KlCstStmtList*, ifcst->if_block));
    if (!ifcst->else_block) {
      if (ifneedclose)
        klgen_emit(gen, klinst_close(stktop), klgen_cstposition(ifcst->if_block));
      klgen_setinstjmppos(gen, cond, klgen_currcodesize(gen));
      return;
    }
    KlCodeVal ifout = klcodeval_jmp(klgen_emit(gen,
                                                   ifneedclose ? klinst_closejmp(stktop, 0)
                                                             : klinst_jmp(0),
                                                   klgen_cstposition(ifcst->if_block)));
    klgen_setinstjmppos(gen, cond, klgen_currcodesize(gen));
    bool elseneedclose = klgen_stmtblock(gen, klcast(KlCstStmtList*, ifcst->else_block));
    if (elseneedclose)
      klgen_emit(gen, klinst_close(stktop), klgen_cstposition(ifcst->if_block));
    klgen_setinstjmppos(gen, ifout, klgen_currcodesize(gen));
  }
}

static void klgen_stmtwhile(KlGenUnit* gen, KlCstStmtWhile* whilecst) {
  KlCodeVal bjmplist = klcodeval_none();
  KlCodeVal cjmplist = klcodeval_jmp(klgen_emit(gen, klinst_jmp(0), klgen_cstposition(whilecst)));
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
      klgen_emit(gen, klinst_close(stktop), klgen_cstposition(whilecst));
  size_t continuepos = klgen_currcodesize(gen);
  KlCodeVal cond = klgen_exprbool(gen, whilecst->cond, true);
  if (klcodeval_isconstant(cond)) {
    if (klcodeval_istrue(cond)) {
      bool cond_has_side_effect = klgen_currcodesize(gen) != continuepos;
      int offset = (int)loopbeginpos - (int)klgen_currcodesize(gen) - 1;
      if (!klinst_inrange(offset, 24))
        klgen_error_fatal(gen, "jump too far, can not generate code");
      klgen_emit(gen, klinst_jmp(offset), klgen_cstposition(whilecst->cond));
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
      klgen_emit(gen, klinst_close(stktop), klgen_cstposition(repeatcst));
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
      klgen_emit(gen, klinst_jmp(offset), klgen_cstposition(repeatcst->cond));
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
                         klcodeval_jmp(klgen_emit(gen, breakinst, klgen_cstposition(breakcst))));
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
                         klcodeval_jmp(klgen_emit(gen, continueinst, klgen_cstposition(continuecst))));
}

static void klgen_stmtreturn(KlGenUnit* gen, KlCstStmtReturn* returncst) {
  kl_assert(klcst_kind(returncst) == KLCST_EXPR_TUPLE, "");
  KlCstTuple* res = klcast(KlCstTuple*, returncst->retval);
  bool needclose = klgen_needclose(gen, gen->reftbl, NULL);
  size_t stktop = klgen_stacktop(gen);
  if (res->nelem == 1) {
    KlCodeVal retval;
    size_t nres = klgen_trytakeall(gen, returncst->retval, &retval);
    kl_assert(retval.kind == KLVAL_STACK, "");
    kl_assert(nres == 1 || nres == KLINST_VARRES, "");
    if (needclose)
      klgen_emit(gen, klinst_close(0), klgen_cstposition(returncst));
    KlInstruction returninst = nres == KLINST_VARRES ? klinst_return(retval.index, nres)
                                                     : klinst_return1(retval.index);
    klgen_emit(gen, returninst, klgen_cstposition(returncst));
  } else {
    size_t argbase = klgen_stacktop(gen);
    size_t nres = klgen_passargs(gen, klcst(res));
    if (needclose)
      klgen_emit(gen, klinst_close(0), klgen_cstposition(returncst));
    KlInstruction returninst = nres == 0 ? klinst_return0() :
                               nres == 1 ? klinst_return1(argbase)
                                         : klinst_return(argbase, nres);
    klgen_emit(gen, returninst, klgen_cstposition(returncst));
  }
  klgen_stackfree(gen, stktop);
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
  klgen_exprtarget_noconst(gen, iforcst->begin, forbase);
  // KlCodeVal begin = klcodeval_stack(forbase);
  klgen_exprtarget_noconst(gen, iforcst->end, forbase + 1);
  if (iforcst->step) {
    klgen_exprtarget_noconst(gen, iforcst->step, forbase + 2);
  } else {
    klgen_emitloadnils(gen, forbase + 2, 1, klgen_position(klcst_end(iforcst->end), klcst_end(iforcst->end)));
    klgen_stackalloc1(gen);
  }
  kl_assert(klgen_stacktop(gen) == forbase + 3, "");

  klgen_mergejmp_maynone(gen, gen->info.breakjmp,
                         klcodeval_jmp(klgen_emit(gen, klinst_iforprep(forbase, 0), klgen_cstposition(iforcst))));
  klgen_pushsymtbl(gen);  /* enter a new scope here */
  size_t looppos = klgen_currcodesize(gen);
  kl_assert(klcst_kind(iforcst->lval) == KLCST_EXPR_TUPLE && klcast(KlCstTuple*, iforcst->lval)->nelem == 1, "");

  KlCst* pattern = klgen_exprpromotion(klcast(KlCstTuple*, iforcst->lval)->elems[0]);
  if ((klcst_kind(pattern) == KLCST_EXPR_ID)) {
    klgen_newsymbol(gen, klcast(KlCstIdentifier*, pattern)->id, forbase, klgen_cstposition(pattern));
  } else {  /* else is pattern deconstruction */
    klgen_pattern_deconstruct_tostktop(gen, pattern, forbase);
    klgen_pattern_newsymbol(gen, pattern, forbase + 3);
    kl_assert(forbase + 3 + klgen_pattern_count_result(gen, pattern) == klgen_stacktop(gen), "");
  }

  klgen_stmtlist(gen, klcast(KlCstStmtList*, iforcst->block));
  if (gen->symtbl->info.referenced)
      klgen_emit(gen, klinst_close(forbase), klgen_cstposition(iforcst));
  klgen_popsymtbl(gen);   /* close the scope */
  klgen_setinstjmppos(gen, cjmplist, klgen_currcodesize(gen));
  klgen_emit(gen, klinst_iforloop(forbase, looppos - klgen_currcodesize(gen) - 1), klgen_cstposition(iforcst));
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
  KlCst** patterns = klcast(KlCstTuple*, vforcst->lvals)->elems;
  size_t npattern = klcast(KlCstTuple*, vforcst->lvals)->nelem;
  KlCodeVal step = klcodeval_integer(npattern);
  klgen_putonstktop(gen, &step, klgen_cstposition(vforcst));
  klgen_mergejmp_maynone(gen, gen->info.breakjmp,
                         klcodeval_jmp(klgen_emit(gen, klinst_vforprep(forbase, 0), klgen_cstposition(vforcst))));
  size_t looppos = klgen_currcodesize(gen);
  klgen_stackalloc1(gen); /* forbase + 0: step, forbase + 1: index */
  klgen_pushsymtbl(gen);  /* begin a new scope */

  klgen_stackalloc(gen, npattern);
  for (KlCst** ppattern = patterns + npattern - 1; ppattern != patterns; --ppattern) {
    KlCst* pattern = klgen_exprpromotion(*ppattern);
    size_t valstkid = forbase + 2 + (ppattern - patterns);
    if (klcst_kind(pattern) == KLCST_EXPR_ID) {
      klgen_newsymbol(gen, klcast(KlCstIdentifier*, pattern)->id, valstkid, klgen_cstposition(pattern));
    } else {
      size_t newsymbol_base = klgen_stacktop(gen);
      klgen_pattern_deconstruct_tostktop(gen, pattern, valstkid);
      klgen_pattern_newsymbol(gen, pattern, newsymbol_base);
    }
  }

  klgen_stmtlist(gen, klcast(KlCstStmtList*, vforcst->block));
  if (gen->symtbl->info.referenced)
      klgen_emit(gen, klinst_close(forbase), klgen_cstposition(vforcst));
  klgen_popsymtbl(gen);   /* close the scope */
  klgen_setinstjmppos(gen, cjmplist, klgen_currcodesize(gen));
  klgen_emit(gen, klinst_vforloop(forbase, looppos - klgen_currcodesize(gen) - 1), klgen_cstposition(vforcst));
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
  size_t iterable = forbase;
  klgen_exprtarget_noconst(gen, gforcst->expr, iterable);
  klgen_stackfree(gen, forbase);
  KlConstant constant = { .type = KL_STRING, .string = gen->string.itermethod };
  KlConEntry* conent = klcontbl_get(gen->contbl, &constant);
  klgen_oomifnull(gen, conent);
  klgen_emitmethod(gen, iterable, conent->index, 0, 3, forbase, klgen_cstposition(gforcst));
  klgen_mergejmp_maynone(gen, gen->info.continuejmp,
                         klcodeval_jmp(klgen_emit(gen, klinst_jmp(0), klgen_cstposition(gforcst))));
  size_t looppos = klgen_currcodesize(gen);
  klgen_pushsymtbl(gen);    /* begin a new scope */
  klgen_stackalloc(gen, 3); /* forbase: iteration function, forbase + 1: static state, forbase + 2: index state */

  KlCst** patterns = klcast(KlCstTuple*, gforcst->lvals)->elems;
  size_t npattern = klcast(KlCstTuple*, gforcst->lvals)->nelem;
  klgen_stackalloc(gen, npattern);
  for (KlCst** ppattern = patterns + npattern - 1; ppattern != patterns; --ppattern) {
    KlCst* pattern = klgen_exprpromotion(*ppattern);
    size_t valstkid = forbase + 3 + (ppattern - patterns);
    if (klcst_kind(pattern) == KLCST_EXPR_ID) {
      klgen_newsymbol(gen, klcast(KlCstIdentifier*, pattern)->id, valstkid, klgen_cstposition(pattern));
    } else {
      size_t newsymbol_base = klgen_stacktop(gen);
      klgen_pattern_deconstruct_tostktop(gen, pattern, valstkid);
      klgen_pattern_newsymbol(gen, pattern, newsymbol_base);
    }
  }

  klgen_stmtlist(gen, klcast(KlCstStmtList*, gforcst->block));
  if (gen->symtbl->info.referenced)
      klgen_emit(gen, klinst_close(forbase), klgen_cstposition(gforcst));
  klgen_popsymtbl(gen);   /* close the scope */
  klgen_setinstjmppos(gen, cjmplist, klgen_currcodesize(gen));
  klgen_emit(gen, klinst_gforloop(forbase, npattern + 2), klgen_cstposition(gforcst));
  klgen_emit(gen, klinst_truejmp(forbase + 1, looppos - klgen_currcodesize(gen) - 1), klgen_cstposition(gforcst));
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
