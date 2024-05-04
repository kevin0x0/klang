#include "include/code/klgen_stmt.h"
#include "include/code/klcodeval.h"
#include "include/code/klcontbl.h"
#include "include/code/klgen.h"
#include "include/code/klgen_expr.h"
#include "include/code/klgen_exprbool.h"
#include "include/code/klgen_pattern.h"
#include "include/code/klsymtbl.h"
#include "include/ast/klast.h"


bool klgen_stmtblock(KlGenUnit *gen, KlAstStmtList *stmtlist) {
  klgen_pushsymtbl(gen);
  klgen_stmtlist(gen, stmtlist);
  bool needclose = gen->symtbl->info.referenced;
  klgen_popsymtbl(gen);
  return needclose;
}

void klgen_stmtlistpure(KlGenUnit* gen, KlAstStmtList* stmtlist) {
  KlCodeVal* prev_bjmp = gen->jmpinfo.breakjmp;
  KlCodeVal* prev_cjmp = gen->jmpinfo.continuejmp;
  KlSymTbl* prev_bscope = gen->jmpinfo.break_scope;
  KlSymTbl* prev_cscope = gen->jmpinfo.continue_scope;
  gen->jmpinfo.breakjmp = NULL;
  gen->jmpinfo.continuejmp = NULL;
  gen->jmpinfo.break_scope = NULL;
  gen->jmpinfo.continue_scope = NULL;
  klgen_stmtlist(gen, stmtlist);
  gen->jmpinfo.breakjmp = prev_bjmp;
  gen->jmpinfo.continuejmp = prev_cjmp;
  gen->jmpinfo.break_scope = prev_bscope;
  gen->jmpinfo.continue_scope = prev_cscope;
}

bool klgen_stmtblockpure(KlGenUnit* gen, KlAstStmtList* stmtlist) {
  KlCodeVal* prev_bjmp = gen->jmpinfo.breakjmp;
  KlCodeVal* prev_cjmp = gen->jmpinfo.continuejmp;
  KlSymTbl* prev_bscope = gen->jmpinfo.break_scope;
  KlSymTbl* prev_cscope = gen->jmpinfo.continue_scope;
  gen->jmpinfo.breakjmp = NULL;
  gen->jmpinfo.continuejmp = NULL;
  gen->jmpinfo.break_scope = NULL;
  gen->jmpinfo.continue_scope = NULL;
  bool needclose = klgen_stmtblock(gen, stmtlist);
  gen->jmpinfo.breakjmp = prev_bjmp;
  gen->jmpinfo.continuejmp = prev_cjmp;
  gen->jmpinfo.break_scope = prev_bscope;
  gen->jmpinfo.continue_scope = prev_cscope;
  return needclose;
}

static void klgen_deconstruct_to_stktop(KlGenUnit* gen, KlAst** patterns, size_t npattern, KlAst** rvals, size_t nrval, KlFilePosition filepos) {
  size_t nfastassign = 0;
  for (; nfastassign < npattern; ++nfastassign) {
    if (klast_kind(patterns[nfastassign]) != KLCST_EXPR_ID)
      break;
  }
  if (nfastassign == npattern) {
    klgen_exprlist_raw(gen, rvals, nrval, nfastassign, filepos);
  } else {  /* now nfastassign < npattern */
    if (nfastassign == npattern - 1) {
      klgen_exprlist_raw(gen, rvals, nrval, npattern, filepos);
      if (klgen_pattern_fastbinding(gen, patterns[npattern - 1]))
        return;
      size_t nreserved = klgen_pattern_count_result(gen, patterns[npattern - 1]);
      kl_assert(klgen_stacktop(gen) > 0, "");
      size_t lastval = klgen_stacktop(gen) - 1;
      if (nreserved != 0) {
        klgen_emitmove(gen, lastval + nreserved, lastval, 1, filepos);
        klgen_stackalloc(gen, nreserved);
      }
      klgen_pattern_binding(gen, patterns[npattern - 1], lastval + nreserved);
    } else if (nfastassign <= nrval) {
      klgen_exprlist_raw(gen, rvals, nfastassign, nfastassign, filepos);
      size_t nreserved = klgen_patterns_count_result(gen, patterns + nfastassign, npattern - nfastassign);
      klgen_stackalloc(gen, nreserved);
      size_t target = klgen_stacktop(gen);
      klgen_exprlist_raw(gen, rvals + nfastassign, nrval - nfastassign, npattern - nfastassign, filepos);
      size_t count = npattern;
      while (count-- > nfastassign)
        target = klgen_pattern_binding(gen, patterns[count], target);
    } else {
      size_t oristktop = klgen_stacktop(gen);
      klgen_exprlist_raw(gen, rvals, nrval, npattern, filepos);
      size_t nreserved = klgen_patterns_count_result(gen, patterns + nfastassign, npattern - nfastassign);
      if (nreserved != 0) {
        klgen_stackalloc(gen, nreserved);
        klgen_emitmove(gen, oristktop + nfastassign + nreserved, oristktop + nfastassign, npattern - nfastassign, filepos);
      }
      size_t target = oristktop + nfastassign + nreserved;
      size_t count = npattern;
      while (count-- > nfastassign)
        target = klgen_pattern_binding(gen, patterns[count], target);
    }
  }
}

static void klgen_stmtlet(KlGenUnit* gen, KlAstStmtLet* letast) {
  KlAstTuple* lvals = klcast(KlAstTuple*, letast->lvals);
  KlAstTuple* rvals = klcast(KlAstTuple*, letast->rvals);
  size_t newsymbol_base = klgen_stacktop(gen);
  klgen_deconstruct_to_stktop(gen, lvals->elems, lvals->nelem, rvals->elems, rvals->nelem, klgen_astposition(rvals));
  klgen_patterns_newsymbol(gen, lvals->elems, lvals->nelem, newsymbol_base);
}

static void klgen_singleassign(KlGenUnit* gen, KlAst* lval, KlAst* rval) {
  if (klast_kind(lval) == KLCST_EXPR_ID) {
    KlAstIdentifier* id = klcast(KlAstIdentifier*, lval);
    KlSymbol* symbol = klgen_getsymbol(gen, id->id);
    if (!symbol) {  /* global variable */
      size_t stktop = klgen_stacktop(gen);
      KlCodeVal res = klgen_expr(gen, rval);
      klgen_putonstack(gen, &res, klgen_astposition(rval));
      size_t conidx = klgen_newstring(gen, id->id);
      klgen_emit(gen, klinst_storeglobal(res.index, conidx), klgen_astposition(lval));
      klgen_stackfree(gen, stktop);
    } else if (symbol->attr.kind == KLVAL_REF) {
      size_t stktop = klgen_stacktop(gen);
      KlCodeVal res = klgen_expr(gen, rval);
      klgen_putonstack(gen, &res, klgen_astposition(rval));
      klgen_emit(gen, klinst_storeref(res.index, symbol->attr.idx), klgen_astposition(lval));
      klgen_stackfree(gen, stktop);
    } else {
      kl_assert(symbol->attr.kind == KLVAL_STACK, "");
      kl_assert(symbol->attr.idx < klgen_stacktop(gen), "");
      KlCodeVal res = klgen_exprtarget(gen, rval, symbol->attr.idx);
      if (klcodeval_isconstant(res))
        klgen_loadval(gen, symbol->attr.idx, res, klgen_astposition(lval));
    }
  } else if (klast_kind(lval) == KLCST_EXPR_POST && klcast(KlAstPost*, lval)->op == KLTK_INDEX) {
    size_t stktop = klgen_stacktop(gen);
    KlAstPost* indexast = klcast(KlAstPost*, lval);
    KlAst* indexableast = indexast->operand;
    KlAst* keyast = indexast->post;
    KlCodeVal val = klgen_expr(gen, rval);
    klgen_putonstack(gen, &val, klgen_astposition(rval));
    KlCodeVal indexable = klgen_expr(gen, indexableast);
    klgen_putonstack(gen, &indexable, klgen_astposition(indexableast));
    KlCodeVal key = klgen_expr(gen, keyast);
    if (key.kind != KLVAL_INTEGER || !klinst_inrange(key.intval, 8))
      klgen_putonstack(gen, &key, klgen_astposition(keyast));
    if (key.kind == KLVAL_INTEGER) {
      klgen_emit(gen, klinst_indexasi(val.index, indexable.index, key.intval), klgen_astposition(lval));
    } else {
      klgen_emit(gen, klinst_indexas(val.index, indexable.index, key.index), klgen_astposition(lval));
    }
    klgen_stackfree(gen, stktop);
  } else if (klast_kind(lval) == KLCST_EXPR_DOT) {
    size_t stktop = klgen_stacktop(gen);
    KlAstDot* dotast = klcast(KlAstDot*, lval);
    KlAst* objast = dotast->operand;
    KlCodeVal val = klgen_expr(gen, rval);
    klgen_putonstack(gen, &val, klgen_astposition(rval));
    KlCodeVal obj = klgen_expr(gen, objast);
    size_t conidx = klgen_newstring(gen, dotast->field);
    if (klinst_inurange(conidx, 8)) {
      if (obj.kind == KLVAL_REF) {
        klgen_emit(gen, klinst_refsetfieldc(val.index, obj.index, conidx), klgen_astposition(lval));
      } else {
        klgen_putonstack(gen, &obj, klgen_astposition(objast));
        klgen_emit(gen, klinst_setfieldc(val.index, obj.index, conidx), klgen_astposition(lval));
      }
    } else {
      size_t tmp = klgen_stackalloc1(gen);
      klgen_emit(gen, klinst_loadc(tmp, conidx), klgen_astposition(lval));
      if (obj.kind == KLVAL_REF) {
        klgen_emit(gen, klinst_refsetfieldr(val.index, obj.index, tmp), klgen_astposition(lval));
      } else {
        klgen_putonstack(gen, &obj, klgen_astposition(objast));
        klgen_emit(gen, klinst_setfieldr(val.index, obj.index, tmp), klgen_astposition(lval));
      }
    }
    klgen_stackfree(gen, stktop);
  } else {
    klgen_error(gen, klast_begin(lval), klast_end(lval), "can not be evaluated to an lvalue");
  }
}

void klgen_assignfrom(KlGenUnit* gen, KlAst* lval, size_t stkid) {
  if (klast_kind(lval) == KLCST_EXPR_ID) {
    KlAstIdentifier* id = klcast(KlAstIdentifier*, lval);
    KlSymbol* symbol = klgen_getsymbol(gen, id->id);
    if (!symbol) {  /* global variable */
      size_t conidx = klgen_newstring(gen, id->id);
      klgen_emit(gen, klinst_storeglobal(stkid, conidx), klgen_astposition(lval));
    } else if (symbol->attr.kind == KLVAL_REF) {
      klgen_emit(gen, klinst_storeref(stkid, symbol->attr.idx), klgen_astposition(lval));
    } else {
      kl_assert(symbol->attr.kind == KLVAL_STACK, "");
      kl_assert(symbol->attr.idx < klgen_stacktop(gen), "");
      kl_assert(symbol->attr.idx != stkid, "");
      klgen_emitmove(gen, symbol->attr.idx, stkid, 1, klgen_astposition(lval));
    }
  } else if (klast_kind(lval) == KLCST_EXPR_POST && klcast(KlAstPost*, lval)->op == KLTK_INDEX) {
    size_t stktop = klgen_stacktop(gen);
    KlAstPost* indexast = klcast(KlAstPost*, lval);
    KlAst* indexableast = indexast->operand;
    KlAst* keyast = indexast->post;
    KlCodeVal indexable = klgen_expr(gen, indexableast);
    klgen_putonstack(gen, &indexable, klgen_astposition(indexableast));
    KlCodeVal key = klgen_expr(gen, keyast);
    if (key.kind != KLVAL_INTEGER || !klinst_inrange(key.intval, 8))
      klgen_putonstack(gen, &key, klgen_astposition(keyast));
    if (key.kind == KLVAL_INTEGER) {
      klgen_emit(gen, klinst_indexasi(stkid, indexable.index, key.intval), klgen_astposition(lval));
    } else {
      klgen_emit(gen, klinst_indexas(stkid, indexable.index, key.index), klgen_astposition(lval));
    }
    klgen_stackfree(gen, stktop);
  } else if (klast_kind(lval) == KLCST_EXPR_DOT) {
    size_t stktop = klgen_stacktop(gen);
    KlAstDot* dotast = klcast(KlAstDot*, lval);
    KlAst* objast = dotast->operand;
    KlCodeVal obj = klgen_expr(gen, objast);
    size_t conidx = klgen_newstring(gen, dotast->field);
    if (klinst_inurange(conidx, 8)) {
      if (obj.kind == KLVAL_REF) {
        klgen_emit(gen, klinst_refsetfieldc(stkid, obj.index, conidx), klgen_astposition(lval));
      } else {
        klgen_putonstack(gen, &obj, klgen_astposition(objast));
        klgen_emit(gen, klinst_setfieldc(stkid, obj.index, conidx), klgen_astposition(lval));
      }
    } else {
      size_t tmp = klgen_stackalloc1(gen);
      klgen_emit(gen, klinst_loadc(tmp, conidx), klgen_astposition(lval));
      if (obj.kind == KLVAL_REF) {
        klgen_emit(gen, klinst_refsetfieldr(stkid, obj.index, tmp), klgen_astposition(lval));
      } else {
        klgen_putonstack(gen, &obj, klgen_astposition(objast));
        klgen_emit(gen, klinst_setfieldr(stkid, obj.index, tmp), klgen_astposition(lval));
      }
    }
    klgen_stackfree(gen, stktop);
  } else {
    klgen_error(gen, klast_begin(lval), klast_end(lval), "can not be evaluated to an lvalue");
  }
}

static inline bool klgen_canassign(KlAst* lval) {
  return (klast_kind(lval) == KLCST_EXPR_ID   ||
          klast_kind(lval) == KLCST_EXPR_DOT  ||
          (klast_kind(lval) == KLCST_EXPR_POST &&
           klcast(KlAstPost*, lval)->op == KLTK_INDEX));
}

static void klgen_stmtassign(KlGenUnit* gen, KlAstStmtAssign* assignast) {
  KlAst** patterns = klcast(KlAstTuple*, assignast->lvals)->elems;
  size_t npattern = klcast(KlAstTuple*, assignast->lvals)->nelem;
  KlAst** rvals = klcast(KlAstTuple*, assignast->rvals)->elems;
  size_t nrval = klcast(KlAstTuple*, assignast->rvals)->nelem;
  kl_assert(npattern != 0, "");
  kl_assert(nrval != 0, "");
  size_t base = klgen_stacktop(gen);
  KlFilePosition rvals_pos = klgen_astposition(assignast->rvals);
  if (nrval != npattern) {
    klgen_deconstruct_to_stktop(gen, patterns, npattern, rvals, nrval, rvals_pos);
    klgen_patterns_do_assignment(gen, patterns, npattern);
    klgen_stackfree(gen, base);
  } else {
    for (size_t i = 0; i < npattern; ++i) {
      if (!klgen_canassign(patterns[i])) {
        klgen_deconstruct_to_stktop(gen, patterns, npattern, rvals, nrval, rvals_pos);
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

/* if success, return true, else return false */
static bool klgen_stmttryfastjmp_inif(KlGenUnit* gen, KlAstStmtIf* ifast) {
  KlAstStmtList* block = klcast(KlAstStmtList*, ifast->if_block);
  if (block->nstmt != 1) return false;
  if (klast_kind(block->stmts[block->nstmt - 1]) != KLCST_STMT_BREAK &&
      klast_kind(block->stmts[block->nstmt - 1]) != KLCST_STMT_CONTINUE) {
    return false;
  }
  KlAst* stmtast = block->stmts[block->nstmt - 1];
  KlSymTbl* endtbl = klast_kind(stmtast) == KLCST_STMT_BREAK ? gen->jmpinfo.break_scope : gen->jmpinfo.continue_scope;
  KlCodeVal* jmplist = klast_kind(stmtast) == KLCST_STMT_BREAK ? gen->jmpinfo.breakjmp : gen->jmpinfo.continuejmp;
  if (kl_unlikely(!jmplist)) {
    klgen_error(gen, klast_begin(stmtast), klast_end(stmtast), "break and continue statement is not allowed here");
    return true;
  }
  if (klgen_needclose(gen, endtbl, NULL))
    return false;
  KlCodeVal cond = klgen_exprbool(gen, ifast->cond, true);
  if (klcodeval_isconstant(cond)) {
    if (klcodeval_isfalse(cond)) return true;
    klgen_mergejmplist_maynone(gen, jmplist,
                           klcodeval_jmplist(klgen_emit(gen, klinst_jmp(0), klgen_astposition(stmtast))));
  } else {
    klgen_mergejmplist_maynone(gen, jmplist, cond);
  }
  return true;
}

static void klgen_stmtif(KlGenUnit* gen, KlAstStmtIf* ifast) {
  if (klgen_stmttryfastjmp_inif(gen, ifast)) return;

  KlCodeVal cond = klgen_exprbool(gen, ifast->cond, false);
  if (klcodeval_isconstant(cond)) {
    if (klcodeval_istrue(cond)) {
      size_t stktop = klgen_stacktop(gen);
      bool needclose = klgen_stmtblock(gen, klcast(KlAstStmtList*, ifast->if_block));
      if (needclose)
        klgen_emit(gen, klinst_close(stktop), klgen_astposition(ifast->if_block));
      return;
    } else {
      if (!ifast->else_block) return;
      size_t stktop = klgen_stacktop(gen);
      bool needclose = klgen_stmtblock(gen, klcast(KlAstStmtList*, ifast->else_block));
      if (needclose)
        klgen_emit(gen, klinst_close(stktop), klgen_astposition(ifast->else_block));
      return;
    }
  } else {
    size_t stktop = klgen_stacktop(gen);
    bool ifneedclose = klgen_stmtblock(gen, klcast(KlAstStmtList*, ifast->if_block));
    if (!ifast->else_block) {
      if (ifneedclose)
        klgen_emit(gen, klinst_close(stktop), klgen_astposition(ifast->if_block));
      klgen_setinstjmppos(gen, cond, klgen_currcodesize(gen));
      return;
    }
    KlCodeVal ifout = klcodeval_jmplist(klgen_emit(gen,
                                                   ifneedclose ? klinst_closejmp(stktop, 0)
                                                             : klinst_jmp(0),
                                                   klgen_astposition(ifast->if_block)));
    klgen_setinstjmppos(gen, cond, klgen_currcodesize(gen));
    bool elseneedclose = klgen_stmtblock(gen, klcast(KlAstStmtList*, ifast->else_block));
    if (elseneedclose)
      klgen_emit(gen, klinst_close(stktop), klgen_astposition(ifast->if_block));
    klgen_setinstjmppos(gen, ifout, klgen_currcodesize(gen));
  }
}

static void klgen_stmtwhile(KlGenUnit* gen, KlAstStmtWhile* whileast) {
  KlCodeVal bjmplist = klcodeval_none();
  KlCodeVal cjmplist = klcodeval_jmplist(klgen_emit(gen, klinst_jmp(0), klgen_astposition(whileast)));
  KlCodeVal* prev_bjmp = gen->jmpinfo.breakjmp;
  KlCodeVal* prev_cjmp = gen->jmpinfo.continuejmp;
  KlSymTbl* prev_bscope = gen->jmpinfo.break_scope;
  KlSymTbl* prev_cscope = gen->jmpinfo.continue_scope;
  gen->jmpinfo.breakjmp = &bjmplist;
  gen->jmpinfo.continuejmp = &cjmplist;
  gen->jmpinfo.break_scope = gen->symtbl;
  gen->jmpinfo.continue_scope = gen->symtbl;

  size_t stktop = klgen_stacktop(gen);
  size_t loopbeginpos = klgen_currcodesize(gen);
  bool needclose = klgen_stmtblock(gen, klcast(KlAstStmtList*, whileast->block));
  if (needclose)
      klgen_emit(gen, klinst_close(stktop), klgen_astposition(whileast));
  size_t continuepos = klgen_currcodesize(gen);
  KlCodeVal cond = klgen_exprbool(gen, whileast->cond, true);
  if (klcodeval_isconstant(cond)) {
    if (klcodeval_istrue(cond)) {
      bool cond_has_side_effect = klgen_currcodesize(gen) != continuepos;
      int offset = (int)loopbeginpos - (int)klgen_currcodesize(gen) - 1;
      if (!klinst_inrange(offset, 24))
        klgen_error_fatal(gen, "jump too far, can not generate code");
      klgen_emit(gen, klinst_jmp(offset), klgen_astposition(whileast->cond));
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
  gen->jmpinfo.breakjmp = prev_bjmp;
  gen->jmpinfo.continuejmp = prev_cjmp;
  gen->jmpinfo.break_scope = prev_bscope;
  gen->jmpinfo.continue_scope = prev_cscope;
}

static void klgen_stmtrepeat(KlGenUnit* gen, KlAstStmtRepeat* repeatast) {
  KlCodeVal bjmplist = klcodeval_none();
  KlCodeVal cjmplist = klcodeval_none();
  KlCodeVal* prev_bjmp = gen->jmpinfo.breakjmp;
  KlCodeVal* prev_cjmp = gen->jmpinfo.continuejmp;
  KlSymTbl* prev_bscope = gen->jmpinfo.break_scope;
  KlSymTbl* prev_cscope = gen->jmpinfo.continue_scope;
  gen->jmpinfo.breakjmp = &bjmplist;
  gen->jmpinfo.continuejmp = &cjmplist;
  gen->jmpinfo.break_scope = gen->symtbl;
  gen->jmpinfo.continue_scope = gen->symtbl;

  size_t stktop = klgen_stacktop(gen);
  size_t loopbeginpos = klgen_currcodesize(gen);
  bool needclose = klgen_stmtblock(gen, klcast(KlAstStmtList*, repeatast->block));
  if (needclose)
      klgen_emit(gen, klinst_close(stktop), klgen_astposition(repeatast));
  size_t continuepos = klgen_currcodesize(gen);
  KlCodeVal cond = klgen_exprbool(gen, repeatast->cond, false);
  if (klcodeval_isconstant(cond)) {
    if (klcodeval_istrue(cond)) {
      klgen_setinstjmppos(gen, cjmplist, continuepos);
      klgen_setinstjmppos(gen, bjmplist, klgen_currcodesize(gen));
    } else {
      bool cond_has_side_effect = klgen_currcodesize(gen) != continuepos;
      int offset = (int)loopbeginpos - (int)klgen_currcodesize(gen) - 1;
      if (!klinst_inrange(offset, 24))
        klgen_error_fatal(gen, "jump too far, can not generate code");
      klgen_emit(gen, klinst_jmp(offset), klgen_astposition(repeatast->cond));
      klgen_setinstjmppos(gen, cjmplist, cond_has_side_effect ? continuepos : loopbeginpos);
      klgen_setinstjmppos(gen, bjmplist, klgen_currcodesize(gen));
    }
  } else {
    klgen_setinstjmppos(gen, cond, loopbeginpos);
    klgen_setinstjmppos(gen, cjmplist, continuepos);
    klgen_setinstjmppos(gen, bjmplist, klgen_currcodesize(gen));
  }
  gen->jmpinfo.breakjmp = prev_bjmp;
  gen->jmpinfo.continuejmp = prev_cjmp;
  gen->jmpinfo.break_scope = prev_bscope;
  gen->jmpinfo.continue_scope = prev_cscope;
}

static void klgen_stmtbreak(KlGenUnit* gen, KlAstStmtBreak* breakast) {
  if (kl_unlikely(!gen->jmpinfo.breakjmp)) {
    klgen_error(gen, klast_begin(breakast), klast_end(breakast), "break statement is not allowed here");
    return;
  }
  size_t closebase;
  KlInstruction breakinst;
  if (klgen_needclose(gen, gen->jmpinfo.break_scope, &closebase)) {
    breakinst = klinst_closejmp(closebase, 0);
  } else {
    breakinst = klinst_jmp(0);
  }
  klgen_mergejmplist_maynone(gen, gen->jmpinfo.breakjmp,
                         klcodeval_jmplist(klgen_emit(gen, breakinst, klgen_astposition(breakast))));
}

static void klgen_stmtcontinue(KlGenUnit* gen, KlAstStmtContinue* continueast) {
  if (kl_unlikely(!gen->jmpinfo.continuejmp)) {
    klgen_error(gen, klast_begin(continueast), klast_end(continueast), "continue statement is not allowed here");
    return;
  }
  size_t closebase;
  KlInstruction continueinst;
  if (klgen_needclose(gen, gen->jmpinfo.continue_scope, &closebase)) {
    continueinst = klinst_closejmp(closebase, 0);
  } else {
    continueinst = klinst_jmp(0);
  }
  klgen_mergejmplist_maynone(gen, gen->jmpinfo.continuejmp,
                         klcodeval_jmplist(klgen_emit(gen, continueinst, klgen_astposition(continueast))));
}

static void klgen_stmtreturn(KlGenUnit* gen, KlAstStmtReturn* returnast) {
  kl_assert(klast_kind(returnast->retvals) == KLCST_EXPR_TUPLE, "");
  KlAstTuple* res = klcast(KlAstTuple*, returnast->retvals);
  size_t stktop = klgen_stacktop(gen);
  if (res->nelem == 1) {
    KlCodeVal retval;
    size_t nres = klgen_trytakeall(gen, res->elems[0], &retval);
    kl_assert(retval.kind == KLVAL_STACK, "");
    kl_assert(nres == 1 || nres == KLINST_VARRES, "");
    if (klgen_needclose(gen, gen->reftbl, NULL))
      klgen_emit(gen, klinst_close(0), klgen_astposition(returnast));
    KlInstruction returninst = nres == KLINST_VARRES ? klinst_return(retval.index, nres)
                                                     : klinst_return1(retval.index);
    klgen_emit(gen, returninst, klgen_astposition(returnast));
  } else {
    size_t argbase = klgen_stacktop(gen);
    size_t nres = klgen_passargs(gen, res);
    if (klgen_needclose(gen, gen->reftbl, NULL))
      klgen_emit(gen, klinst_close(0), klgen_astposition(returnast));
    KlInstruction returninst = nres == 0 ? klinst_return0() :
                               nres == 1 ? klinst_return1(argbase)
                                         : klinst_return(argbase, nres);
    klgen_emit(gen, returninst, klgen_astposition(returnast));
  }
  klgen_stackfree(gen, stktop);
}

static void klgen_stmtifor(KlGenUnit* gen, KlAstStmtIFor* iforast) {
  KlCodeVal bjmplist = klcodeval_none();
  KlCodeVal cjmplist = klcodeval_none();
  KlCodeVal* prev_bjmp = gen->jmpinfo.breakjmp;
  KlCodeVal* prev_cjmp = gen->jmpinfo.continuejmp;
  KlSymTbl* prev_bscope = gen->jmpinfo.break_scope;
  KlSymTbl* prev_cscope = gen->jmpinfo.continue_scope;
  gen->jmpinfo.breakjmp = &bjmplist;
  gen->jmpinfo.continuejmp = &cjmplist;
  gen->jmpinfo.break_scope = gen->symtbl;
  gen->jmpinfo.continue_scope = gen->symtbl;

  size_t forbase = klgen_stacktop(gen);
  klgen_exprtarget_noconst(gen, iforast->begin, forbase);
  // KlCodeVal begin = klcodeval_stack(forbase);
  klgen_exprtarget_noconst(gen, iforast->end, forbase + 1);
  if (iforast->step) {
    klgen_exprtarget_noconst(gen, iforast->step, forbase + 2);
  } else {
    klgen_emitloadnils(gen, forbase + 2, 1, klgen_position(klast_end(iforast->end), klast_end(iforast->end)));
    klgen_stackalloc1(gen);
  }
  kl_assert(klgen_stacktop(gen) == forbase + 3, "");

  klgen_mergejmplist_maynone(gen, gen->jmpinfo.breakjmp,
                         klcodeval_jmplist(klgen_emit(gen, klinst_iforprep(forbase, 0), klgen_astposition(iforast))));
  klgen_pushsymtbl(gen);  /* enter a new scope here */
  size_t looppos = klgen_currcodesize(gen);
  kl_assert(klast_kind(iforast->lval) == KLCST_EXPR_TUPLE && klcast(KlAstTuple*, iforast->lval)->nelem == 1, "");

  KlAst* pattern = klcast(KlAstTuple*, iforast->lval)->elems[0];
  if ((klast_kind(pattern) == KLCST_EXPR_ID)) {
    klgen_newsymbol(gen, klcast(KlAstIdentifier*, pattern)->id, forbase, klgen_astposition(pattern));
  } else {  /* else is pattern deconstruction */
    klgen_pattern_binding_tostktop(gen, pattern, forbase);
    klgen_pattern_newsymbol(gen, pattern, forbase + 3);
    kl_assert(forbase + 3 + klgen_pattern_count_result(gen, pattern) == klgen_stacktop(gen), "");
  }

  klgen_stmtlist(gen, klcast(KlAstStmtList*, iforast->block));
  if (gen->symtbl->info.referenced)
      klgen_emit(gen, klinst_close(forbase), klgen_astposition(iforast));
  klgen_popsymtbl(gen);   /* close the scope */
  klgen_setinstjmppos(gen, cjmplist, klgen_currcodesize(gen));
  klgen_emit(gen, klinst_iforloop(forbase, looppos - klgen_currcodesize(gen) - 1), klgen_astposition(iforast));
  klgen_setinstjmppos(gen, bjmplist, klgen_currcodesize(gen));
  klgen_stackfree(gen, forbase);

  gen->jmpinfo.breakjmp = prev_bjmp;
  gen->jmpinfo.continuejmp = prev_cjmp;
  gen->jmpinfo.break_scope = prev_bscope;
  gen->jmpinfo.continue_scope = prev_cscope;
}

static void klgen_stmtvfor(KlGenUnit* gen, KlAstStmtVFor* vforast) {
  KlCodeVal bjmplist = klcodeval_none();
  KlCodeVal cjmplist = klcodeval_none();
  KlCodeVal* prev_bjmp = gen->jmpinfo.breakjmp;
  KlCodeVal* prev_cjmp = gen->jmpinfo.continuejmp;
  KlSymTbl* prev_bscope = gen->jmpinfo.break_scope;
  KlSymTbl* prev_cscope = gen->jmpinfo.continue_scope;
  gen->jmpinfo.breakjmp = &bjmplist;
  gen->jmpinfo.continuejmp = &cjmplist;
  gen->jmpinfo.break_scope = gen->symtbl;
  gen->jmpinfo.continue_scope = gen->symtbl;

  size_t forbase = klgen_stacktop(gen);
  KlAst** patterns = klcast(KlAstTuple*, vforast->lvals)->elems;
  size_t npattern = klcast(KlAstTuple*, vforast->lvals)->nelem;
  KlCodeVal step = klcodeval_integer(npattern);
  klgen_putonstktop(gen, &step, klgen_astposition(vforast));
  klgen_mergejmplist_maynone(gen, gen->jmpinfo.breakjmp,
                         klcodeval_jmplist(klgen_emit(gen, klinst_vforprep(forbase, 0), klgen_astposition(vforast))));
  size_t looppos = klgen_currcodesize(gen);
  klgen_stackalloc1(gen); /* forbase + 0: step, forbase + 1: index */
  klgen_pushsymtbl(gen);  /* begin a new scope */

  klgen_stackalloc(gen, npattern);
  for (size_t i = npattern; i--;) {
    KlAst* pattern = patterns[i];
    size_t valstkid = forbase + 2 + i;
    if (klast_kind(pattern) == KLCST_EXPR_ID) {
      klgen_newsymbol(gen, klcast(KlAstIdentifier*, pattern)->id, valstkid, klgen_astposition(pattern));
    } else {
      size_t newsymbol_base = klgen_stacktop(gen);
      klgen_pattern_binding_tostktop(gen, pattern, valstkid);
      klgen_pattern_newsymbol(gen, pattern, newsymbol_base);
    }
  }

  klgen_stmtlist(gen, klcast(KlAstStmtList*, vforast->block));
  if (gen->symtbl->info.referenced)
      klgen_emit(gen, klinst_close(forbase), klgen_astposition(vforast));
  klgen_popsymtbl(gen);   /* close the scope */
  klgen_setinstjmppos(gen, cjmplist, klgen_currcodesize(gen));
  klgen_emit(gen, klinst_vforloop(forbase, looppos - klgen_currcodesize(gen) - 1), klgen_astposition(vforast));
  klgen_setinstjmppos(gen, bjmplist, klgen_currcodesize(gen));
  klgen_stackfree(gen, forbase);

  gen->jmpinfo.breakjmp = prev_bjmp;
  gen->jmpinfo.continuejmp = prev_cjmp;
  gen->jmpinfo.break_scope = prev_bscope;
  gen->jmpinfo.continue_scope = prev_cscope;
}

static void klgen_stmtgfor(KlGenUnit* gen, KlAstStmtGFor* gforast) {
  KlCodeVal bjmplist = klcodeval_none();
  KlCodeVal cjmplist = klcodeval_none();
  KlCodeVal* prev_bjmp = gen->jmpinfo.breakjmp;
  KlCodeVal* prev_cjmp = gen->jmpinfo.continuejmp;
  KlSymTbl* prev_bscope = gen->jmpinfo.break_scope;
  KlSymTbl* prev_cscope = gen->jmpinfo.continue_scope;
  gen->jmpinfo.breakjmp = &bjmplist;
  gen->jmpinfo.continuejmp = &cjmplist;
  gen->jmpinfo.break_scope = gen->symtbl;
  gen->jmpinfo.continue_scope = gen->symtbl;

  size_t forbase = klgen_stacktop(gen);
  size_t iterable = forbase;
  klgen_exprtarget_noconst(gen, gforast->expr, iterable);
  klgen_stackfree(gen, forbase);
  size_t conidx = klgen_newstring(gen, gen->strings->itermethod);
  klgen_emitmethod(gen, iterable, conidx, 0, 3, forbase, klgen_astposition(gforast));
  klgen_mergejmplist_maynone(gen, gen->jmpinfo.continuejmp,
                         klcodeval_jmplist(klgen_emit(gen, klinst_jmp(0), klgen_astposition(gforast))));
  size_t looppos = klgen_currcodesize(gen);
  klgen_pushsymtbl(gen);    /* begin a new scope */
  klgen_stackalloc(gen, 3); /* forbase: iteration function, forbase + 1: static state, forbase + 2: index state */

  KlAst** patterns = klcast(KlAstTuple*, gforast->lvals)->elems;
  size_t npattern = klcast(KlAstTuple*, gforast->lvals)->nelem;
  klgen_stackalloc(gen, npattern);
  for (size_t i = npattern; i--;) {
    KlAst* pattern = patterns[i];
    size_t valstkid = forbase + 3 + i;
    if (klast_kind(pattern) == KLCST_EXPR_ID) {
      klgen_newsymbol(gen, klcast(KlAstIdentifier*, pattern)->id, valstkid, klgen_astposition(pattern));
    } else {
      size_t newsymbol_base = klgen_stacktop(gen);
      klgen_pattern_binding_tostktop(gen, pattern, valstkid);
      klgen_pattern_newsymbol(gen, pattern, newsymbol_base);
    }
  }

  klgen_stmtlist(gen, klcast(KlAstStmtList*, gforast->block));
  if (gen->symtbl->info.referenced)
      klgen_emit(gen, klinst_close(forbase), klgen_astposition(gforast));
  klgen_popsymtbl(gen);   /* close the scope */
  klgen_setinstjmppos(gen, cjmplist, klgen_currcodesize(gen));
  klgen_emit(gen, klinst_gforloop(forbase, npattern + 2), klgen_astposition(gforast));
  klgen_emit(gen, klinst_truejmp(forbase + 1, looppos - klgen_currcodesize(gen) - 1), klgen_astposition(gforast));
  klgen_setinstjmppos(gen, bjmplist, klgen_currcodesize(gen));
  klgen_stackfree(gen, forbase);

  gen->jmpinfo.breakjmp = prev_bjmp;
  gen->jmpinfo.continuejmp = prev_cjmp;
  gen->jmpinfo.break_scope = prev_bscope;
  gen->jmpinfo.continue_scope = prev_cscope;
}

void klgen_stmtlist(KlGenUnit* gen, KlAstStmtList* ast) {
  KlAst** endstmt = ast->stmts + ast->nstmt;
  for (KlAst** pstmt = ast->stmts; pstmt != endstmt; ++pstmt) {
    KlAst* stmt = *pstmt;
    switch (klast_kind(stmt)) {
      case KLCST_STMT_LET: {
        klgen_stmtlet(gen, klcast(KlAstStmtLet*, stmt));
        break;
      }
      case KLCST_STMT_ASSIGN: {
        klgen_stmtassign(gen, klcast(KlAstStmtAssign*, stmt));
        break;
      }
      case KLCST_STMT_EXPR: {
        klgen_multival(gen, klast(klcast(KlAstStmtExpr*, stmt)->exprlist), 0, klgen_stacktop(gen));
        break;
      }
      case KLCST_STMT_BLOCK: {
        klgen_stmtblock(gen, klcast(KlAstStmtList*, stmt));
        break;
      }
      case KLCST_STMT_IF: {
        klgen_stmtif(gen, klcast(KlAstStmtIf*, stmt));
        break;
      }
      case KLCST_STMT_IFOR: {
        klgen_stmtifor(gen, klcast(KlAstStmtIFor*, stmt));
        break;
      }
      case KLCST_STMT_VFOR: {
        klgen_stmtvfor(gen, klcast(KlAstStmtVFor*, stmt));
        break;
      }
      case KLCST_STMT_GFOR: {
        klgen_stmtgfor(gen, klcast(KlAstStmtGFor*, stmt));
        break;
      }
      case KLCST_STMT_WHILE: {
        klgen_stmtwhile(gen, klcast(KlAstStmtWhile*, stmt));
        break;
      }
      case KLCST_STMT_REPEAT: {
        klgen_stmtrepeat(gen, klcast(KlAstStmtRepeat*, stmt));
        break;
      }
      case KLCST_STMT_BREAK: {
        klgen_stmtbreak(gen, klcast(KlAstStmtBreak*, stmt));
        break;
      }
      case KLCST_STMT_CONTINUE: {
        klgen_stmtcontinue(gen, klcast(KlAstStmtContinue*, stmt));
        break;
      }
      case KLCST_STMT_RETURN: {
        klgen_stmtreturn(gen, klcast(KlAstStmtReturn*, stmt));
        break;
      }
      default: {
        kl_assert(false, "control flow should not reach here");
        break;
      }
    }
  }
}
