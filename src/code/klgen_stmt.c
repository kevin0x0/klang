#include "include/code/klgen_stmt.h"
#include "include/code/klcodeval.h"
#include "include/code/klgen.h"
#include "include/code/klgen_expr.h"
#include "include/code/klgen_exprbool.h"
#include "include/code/klgen_pattern.h"
#include "include/code/klsymtbl.h"
#include "include/ast/klast.h"
#include "include/common/klinst.h"


bool klgen_stmtblock(KlGenUnit *gen, KlAstStmtList *stmtlist) {
  klgen_pushsymtbl(gen);
  klgen_stmtlist(gen, stmtlist);
  bool needclose = gen->symtbl->info.referenced;
  klgen_popsymtbl(gen);
  return needclose;
}

bool klgen_stmtblockpure(KlGenUnit* gen, KlAstStmtList* stmtlist) {
  KlCodeVal* prev_bjmplist = gen->jmpinfo.breaklist;
  KlCodeVal* prev_cjmplist = gen->jmpinfo.continuelist;
  KlSymTbl* prev_bscope = gen->jmpinfo.break_scope;
  KlSymTbl* prev_cscope = gen->jmpinfo.continue_scope;
  gen->jmpinfo.breaklist = NULL;
  gen->jmpinfo.continuelist = NULL;
  gen->jmpinfo.break_scope = NULL;
  gen->jmpinfo.continue_scope = NULL;
  bool needclose = klgen_stmtblock(gen, stmtlist);
  gen->jmpinfo.breaklist = prev_bjmplist;
  gen->jmpinfo.continuelist = prev_cjmplist;
  gen->jmpinfo.break_scope = prev_bscope;
  gen->jmpinfo.continue_scope = prev_cscope;
  return needclose;
}

static void deconstruct_to_stktop(KlGenUnit* gen, KlAstExpr** patterns, size_t npattern, KlAstExpr** rvals, size_t nrval, KlFilePosition filepos) {
  size_t nfastassign = 0;
  for (; nfastassign < npattern; ++nfastassign) {
    if (!klast_expr_islvalue(patterns[nfastassign]))
      break;
  }
  if (nfastassign == npattern) {
    klgen_exprlist_raw(gen, rvals, nrval, nfastassign, filepos);
    return;
  }

  /* now nfastassign < npattern */
  if (nfastassign == npattern - 1) {
    klgen_exprlist_raw(gen, rvals, nrval, npattern, filepos);
    if (klgen_pattern_fastbinding(gen, patterns[npattern - 1]))
      return;
    size_t nreserved = klgen_pattern_count_result(gen, patterns[npattern - 1]);
    kl_assert(klgen_stacktop(gen) > 0, "");
    KlCStkId lastval = klgen_stacktop(gen) - 1;
    if (nreserved != 0) {
      klgen_emitmove(gen, lastval + nreserved, lastval, 1, filepos);
      klgen_stackalloc(gen, nreserved);
    }
    klgen_pattern_binding(gen, patterns[npattern - 1], lastval + nreserved);
  } else if (nfastassign <= nrval) {
    klgen_exprlist_raw(gen, rvals, nfastassign, nfastassign, filepos);
    size_t nreserved = klgen_patterns_count_result(gen, patterns + nfastassign, npattern - nfastassign);
    klgen_stackalloc(gen, nreserved);
    KlCStkId target = klgen_stacktop(gen);
    klgen_exprlist_raw(gen, rvals + nfastassign, nrval - nfastassign, npattern - nfastassign, filepos);
    size_t count = npattern;
    while (count-- > nfastassign)
      target = klgen_pattern_binding(gen, patterns[count], target);
  } else {
    KlCStkId oristktop = klgen_stacktop(gen);
    klgen_exprlist_raw(gen, rvals, nrval, npattern, filepos);
    size_t nreserved = klgen_patterns_count_result(gen, patterns + nfastassign, npattern - nfastassign);
    if (nreserved != 0) {
      klgen_stackalloc(gen, nreserved);
      klgen_emitmove(gen, oristktop + nfastassign + nreserved, oristktop + nfastassign, npattern - nfastassign, filepos);
    }
    KlCStkId target = oristktop + nfastassign + nreserved;
    size_t count = npattern;
    while (count-- > nfastassign)
      target = klgen_pattern_binding(gen, patterns[count], target);
  }
}

static void gen_localdef(KlGenUnit* gen, KlAstStmtLocalDefinition* localdefast) {
  KlAstExpr* expr = localdefast->expr;
  KlCStkId stktop = klgen_stacktop(gen);
  if (klast_kind(expr) != KLAST_EXPR_FUNC)
    klgen_emitloadnils(gen, stktop, 1, klgen_astposition(localdefast));
  klgen_newsymbol(gen, localdefast->id, stktop, klgen_position(localdefast->idbegin, localdefast->idend));
  klgen_expr_evaluated_to_noconst(gen, localdefast->expr, stktop);
  kl_assert(klgen_stacktop(gen) == stktop + 1, "");
}

static void gen_let(KlGenUnit* gen, KlAstStmtLet* letast) {
  KlAstExprList* lvals = letast->lvals;
  KlAstExprList* rvals = letast->rvals;
  KlCStkId newsymbol_base = klgen_stacktop(gen);
  deconstruct_to_stktop(gen, lvals->exprs, lvals->nexpr, rvals->exprs, rvals->nexpr, klgen_astposition(rvals));
  klgen_patterns_newsymbol(gen, lvals->exprs, lvals->nexpr, newsymbol_base);
}

static void gen_method(KlGenUnit* gen, KlAstStmtMethod* methodast) {
  KlAstDot* lval = methodast->lval;
  KlAstExpr* rval = methodast->rval;
  KlCStkId stktop = klgen_stacktop(gen);
  KlCodeVal val = klgen_expr_onstack(gen, rval);
  KlCodeVal obj = klgen_expr_onstack(gen, lval->operand);
  KlCIdx conidx = klgen_newstring(gen, lval->field);
  if (klinst_inurange(conidx, 8)) {
    klgen_emit(gen, klinst_newmethodc(obj.index, val.index, conidx), klgen_astposition(methodast));
  } else {
    KlCStkId strpos = klgen_stackalloc1(gen);
    klgen_emit(gen, klinst_loadc(strpos, val.index), klgen_astposition(lval));
    klgen_emit(gen, klinst_newmethodr(obj.index, val.index, strpos), klgen_astposition(lval));
  }
  klgen_stackfree(gen, stktop);
}

static void gen__domatching(KlGenUnit* gen, KlAstExpr* pattern, KlCStkId base, KlCStkId matchobj) {
  if (klast_kind(pattern) == KLAST_EXPR_CONSTANT) {
    KlConstant* constant = &klcast(KlAstConstant*, pattern)->con;
    KlFilePosition filepos = klgen_astposition(pattern);
    if (constant->type == KLC_INT) {
      klgen_emit(gen, klinst_inrange(constant->intval, 16)
                      ? klinst_nei(matchobj, constant->intval)
                      : klinst_nec(matchobj, klgen_newinteger(gen, constant->intval)),
                 filepos);
    } else {
      klgen_emit(gen, klinst_nec(matchobj, klgen_newconstant(gen, constant)), filepos);
    }
    KlCPC pc = klgen_emit(gen, klinst_condjmp(true, 0), filepos);
    klgen_mergejmplist_maynone(gen, &gen->jmpinfo.jumpinfo->terminatelist, klcodeval_jmplist(pc));
  } else {
    klgen_pattern_matching_tostktop(gen, pattern, matchobj);
    klgen_pattern_newsymbol(gen, pattern, base);
  }
}

static void gen_match(KlGenUnit* gen, KlAstStmtMatch* stmtmatchast) {
  KlCStkId oristktop = klgen_stacktop(gen);
  KlCodeVal matchobj = klgen_expr(gen, stmtmatchast->matchobj);
  klgen_putonstack(gen, &matchobj, klgen_astposition(stmtmatchast->matchobj));
  KlCStkId currstktop = klgen_stacktop(gen);

  KlCodeVal jmpoutlist = klcodeval_none();
  size_t npattern = stmtmatchast->npattern;
  KlAstExpr** patterns = stmtmatchast->patterns;
  KlAstStmtList** stmtlists = stmtmatchast->stmtlists;
  for (size_t i = 0; i < npattern; ++i) {
    KlGenJumpInfo jmpinfo = {
      .truelist = klcodeval_none(),
      .falselist = klcodeval_none(),
      .terminatelist = klcodeval_none(),
      .prev = gen->jmpinfo.jumpinfo,
    };
    gen->jmpinfo.jumpinfo = &jmpinfo;

    klgen_pushsymtbl(gen);
    gen__domatching(gen, patterns[i], currstktop, matchobj.index);
    klgen_stmtlist(gen, stmtlists[i]);


    if (i != npattern - 1) {
      KlCPC pc = klgen_emit(gen, gen->symtbl->info.referenced ? klinst_closejmp(currstktop, 0)
                                                              : klinst_jmp(0),
                            klgen_astposition(stmtmatchast));
      klgen_mergejmplist_maynone(gen, &jmpoutlist, klcodeval_jmplist(pc));
    } else if (gen->symtbl->info.referenced) {
      klgen_emit(gen, klinst_close(currstktop), klgen_astposition(stmtmatchast));
    }
    klgen_popsymtbl(gen);

    klgen_jumpto(gen, jmpinfo.terminatelist, klgen_getjmptarget(gen));
    gen->jmpinfo.jumpinfo = jmpinfo.prev;
    kl_assert(jmpinfo.truelist.kind == KLVAL_NONE && jmpinfo.falselist.kind == KLVAL_NONE, "");
    klgen_stackfree(gen, currstktop);
  }
  klgen_jumpto(gen, jmpoutlist, klgen_getjmptarget(gen));
  klgen_stackfree(gen, oristktop);
}

static void gen_singleassign(KlGenUnit* gen, KlAstExpr* lval, KlAstExpr* rval) {
  if (klast_kind(lval) == KLAST_EXPR_ID) {
    KlAstIdentifier* id = klcast(KlAstIdentifier*, lval);
    KlSymbol* symbol = klgen_getsymbol(gen, id->id);
    if (!symbol) {  /* global variable */
      KlCStkId stktop = klgen_stacktop(gen);
      KlCodeVal res = klgen_expr(gen, rval);
      klgen_putonstack(gen, &res, klgen_astposition(rval));
      KlCIdx conidx = klgen_newstring(gen, id->id);
      klgen_emit(gen, klinst_storeglobal(res.index, conidx), klgen_astposition(lval));
      klgen_stackfree(gen, stktop);
    } else if (symbol->attr.kind == KLVAL_REF) {
      KlCStkId stktop = klgen_stacktop(gen);
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
  } else if (klast_kind(lval) == KLAST_EXPR_INDEX) {
    KlCStkId stktop = klgen_stacktop(gen);
    KlAstIndex* indexast = klcast(KlAstIndex*, lval);
    KlAstExpr* indexableast = indexast->indexable;
    KlAstExpr* keyast = indexast->index;
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
  } else if (klast_kind(lval) == KLAST_EXPR_DOT) {
    KlCStkId stktop = klgen_stacktop(gen);
    KlAstDot* dotast = klcast(KlAstDot*, lval);
    KlAstExpr* objast = dotast->operand;
    KlCodeVal val = klgen_expr(gen, rval);
    klgen_putonstack(gen, &val, klgen_astposition(rval));
    KlCodeVal obj = klgen_expr(gen, objast);
    KlCIdx conidx = klgen_newstring(gen, dotast->field);
    if (klinst_inurange(conidx, 8)) {
      if (obj.kind == KLVAL_REF) {
        klgen_emit(gen, klinst_refsetfieldc(val.index, obj.index, conidx), klgen_astposition(lval));
      } else {
        klgen_putonstack(gen, &obj, klgen_astposition(objast));
        klgen_emit(gen, klinst_setfieldc(val.index, obj.index, conidx), klgen_astposition(lval));
      }
    } else {
      KlCStkId tmp = klgen_stackalloc1(gen);
      klgen_emit(gen, klinst_loadc(tmp, conidx), klgen_astposition(lval));
      if (obj.kind == KLVAL_REF) {
        klgen_emit(gen, klinst_refsetfieldr(val.index, obj.index, tmp), klgen_astposition(lval));
      } else {
        klgen_putonstack(gen, &obj, klgen_astposition(objast));
        klgen_emit(gen, klinst_setfieldr(val.index, obj.index, tmp), klgen_astposition(lval));
      }
    }
    klgen_stackfree(gen, stktop);
  } else if (klast_kind(lval) == KLAST_EXPR_WILDCARD) {
    /* do nothing */
  } else {
    klgen_error(gen, klast_begin(lval), klast_end(lval), "can not be evaluated to an lvalue");
  }
}

void klgen_assignfrom(KlGenUnit* gen, KlAstExpr* lval, KlCStkId stkid) {
  if (klast_kind(lval) == KLAST_EXPR_ID) {
    KlAstIdentifier* id = klcast(KlAstIdentifier*, lval);
    KlSymbol* symbol = klgen_getsymbol(gen, id->id);
    if (!symbol) {  /* global variable */
      KlCIdx conidx = klgen_newstring(gen, id->id);
      klgen_emit(gen, klinst_storeglobal(stkid, conidx), klgen_astposition(lval));
    } else if (symbol->attr.kind == KLVAL_REF) {
      klgen_emit(gen, klinst_storeref(stkid, symbol->attr.idx), klgen_astposition(lval));
    } else {
      kl_assert(symbol->attr.kind == KLVAL_STACK, "");
      kl_assert(symbol->attr.idx < klgen_stacktop(gen), "");
      kl_assert(symbol->attr.idx != stkid, "");
      klgen_emitmove(gen, symbol->attr.idx, stkid, 1, klgen_astposition(lval));
    }
  } else if (klast_kind(lval) == KLAST_EXPR_INDEX) {
    KlCStkId stktop = klgen_stacktop(gen);
    KlAstIndex* indexast = klcast(KlAstIndex*, lval);
    KlAstExpr* indexableast = indexast->indexable;
    KlAstExpr* keyast = indexast->index;
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
  } else if (klast_kind(lval) == KLAST_EXPR_DOT) {
    KlCStkId stktop = klgen_stacktop(gen);
    KlAstDot* dotast = klcast(KlAstDot*, lval);
    KlAstExpr* objast = dotast->operand;
    KlCodeVal obj = klgen_expr(gen, objast);
    KlCIdx conidx = klgen_newstring(gen, dotast->field);
    if (klinst_inurange(conidx, 8)) {
      if (obj.kind == KLVAL_REF) {
        klgen_emit(gen, klinst_refsetfieldc(stkid, obj.index, conidx), klgen_astposition(lval));
      } else {
        klgen_putonstack(gen, &obj, klgen_astposition(objast));
        klgen_emit(gen, klinst_setfieldc(stkid, obj.index, conidx), klgen_astposition(lval));
      }
    } else {
      KlCStkId tmp = klgen_stackalloc1(gen);
      klgen_emit(gen, klinst_loadc(tmp, conidx), klgen_astposition(lval));
      if (obj.kind == KLVAL_REF) {
        klgen_emit(gen, klinst_refsetfieldr(stkid, obj.index, tmp), klgen_astposition(lval));
      } else {
        klgen_putonstack(gen, &obj, klgen_astposition(objast));
        klgen_emit(gen, klinst_setfieldr(stkid, obj.index, tmp), klgen_astposition(lval));
      }
    }
    klgen_stackfree(gen, stktop);
  } else if (klast_kind(lval) == KLAST_EXPR_WILDCARD) {
    /* do nothing */
  } else {
    klgen_error(gen, klast_begin(lval), klast_end(lval), "can not be evaluated to an lvalue");
  }
}

static void gen_assign(KlGenUnit* gen, KlAstStmtAssign* assignast) {
  KlAstExpr** patterns = assignast->lvals->exprs;
  size_t npattern = assignast->lvals->nexpr;
  KlAstExpr** rvals = assignast->rvals->exprs;
  size_t nrval = assignast->rvals->nexpr;
  kl_assert(npattern != 0, "");
  kl_assert(nrval != 0, "");
  KlCStkId base = klgen_stacktop(gen);
  KlFilePosition rvals_pos = klgen_astposition(assignast->rvals);
  if (nrval != npattern) {
    deconstruct_to_stktop(gen, patterns, npattern, rvals, nrval, rvals_pos);
    klgen_patterns_do_assignment(gen, patterns, npattern);
    klgen_stackfree(gen, base);
  } else {
    for (size_t i = 0; i < npattern; ++i) {
      if (!klast_expr_islvalue(patterns[i])) {
        deconstruct_to_stktop(gen, patterns, npattern, rvals, nrval, rvals_pos);
        klgen_patterns_do_assignment(gen, patterns, npattern);
        klgen_stackfree(gen, base);
        return;
      }
    }
    klgen_exprlist_raw(gen, rvals, nrval - 1, nrval - 1, rvals_pos);
    gen_singleassign(gen, patterns[npattern - 1], rvals[nrval - 1]);
    for (size_t i = nrval - 1; i-- > 0;)
      klgen_assignfrom(gen, patterns[i], base + i);
    klgen_stackfree(gen, base);
  }
}

static bool needclose(KlGenUnit* gen, KlSymTbl* endtbl, KlCStkId* closebase) {
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
static bool tryfastjmp_in_if_stmt(KlGenUnit* gen, KlAstStmtIf* ifast) {
  KlAstStmtList* block = ifast->then_block;
  if (block->nstmt != 1) return false;
  if (klast_kind(block->stmts[block->nstmt - 1]) != KLAST_STMT_BREAK &&
      klast_kind(block->stmts[block->nstmt - 1]) != KLAST_STMT_CONTINUE) {
    return false;
  }
  KlAstStmt* stmtast = block->stmts[block->nstmt - 1];
  KlSymTbl* endtbl = klast_kind(stmtast) == KLAST_STMT_BREAK ? gen->jmpinfo.break_scope : gen->jmpinfo.continue_scope;
  KlCodeVal* jmplist = klast_kind(stmtast) == KLAST_STMT_BREAK ? gen->jmpinfo.breaklist : gen->jmpinfo.continuelist;
  if (kl_unlikely(!jmplist)) {
    klgen_error(gen, klast_begin(stmtast), klast_end(stmtast), "break and continue statement is not allowed here");
    return true;
  }
  if (needclose(gen, endtbl, NULL))
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

static void gen_if(KlGenUnit* gen, KlAstStmtIf* ifast) {
  if (tryfastjmp_in_if_stmt(gen, ifast)) return;

  KlCodeVal falselist = klgen_exprbool(gen, ifast->cond, false);
  if (klcodeval_isconstant(falselist)) {
    if (klcodeval_istrue(falselist)) {
      KlCStkId stktop = klgen_stacktop(gen);
      bool needclose = klgen_stmtblock(gen, ifast->then_block);
      if (needclose)
        klgen_emit(gen, klinst_close(stktop), klgen_astposition(ifast->then_block));
      return;
    } else {
      if (!ifast->else_block) return;
      KlCStkId stktop = klgen_stacktop(gen);
      bool needclose = klgen_stmtblock(gen, ifast->else_block);
      if (needclose)
        klgen_emit(gen, klinst_close(stktop), klgen_astposition(ifast->else_block));
      return;
    }
  } else {
    KlCStkId stktop = klgen_stacktop(gen);
    bool then_needclose = klgen_stmtblock(gen, ifast->then_block);
    if (!ifast->else_block) {
      if (then_needclose)
        klgen_emit(gen, klinst_close(stktop), klgen_astposition(ifast->then_block));
      klgen_jumpto(gen, falselist, klgen_getjmptarget(gen));
      return;
    }
    KlCodeVal thenout = klcodeval_jmplist(klgen_emit(gen,
                                                     then_needclose ? klinst_closejmp(stktop, 0)
                                                                 : klinst_jmp(0),
                                                     klgen_astposition(ifast->then_block)));
    klgen_jumpto(gen, falselist, klgen_getjmptarget(gen));
    bool else_needclose = klgen_stmtblock(gen, ifast->else_block);
    if (else_needclose)
      klgen_emit(gen, klinst_close(stktop), klgen_astposition(ifast->then_block));
    klgen_jumpto(gen, thenout, klgen_getjmptarget(gen));
  }
}

static void gen_while(KlGenUnit* gen, KlAstStmtWhile* whileast) {
  KlCodeVal bjmplist = klcodeval_none();
  KlCodeVal cjmplist = klcodeval_jmplist(klgen_emit(gen, klinst_jmp(0), klgen_astposition(whileast)));
  KlCodeVal* prev_bjmplist = gen->jmpinfo.breaklist;
  KlCodeVal* prev_cjmplist = gen->jmpinfo.continuelist;
  KlSymTbl* prev_bscope = gen->jmpinfo.break_scope;
  KlSymTbl* prev_cscope = gen->jmpinfo.continue_scope;
  gen->jmpinfo.breaklist = &bjmplist;
  gen->jmpinfo.continuelist = &cjmplist;
  gen->jmpinfo.break_scope = gen->symtbl;
  gen->jmpinfo.continue_scope = gen->symtbl;

  KlCStkId stktop = klgen_stacktop(gen);
  KlCPC loopbeginpos = klgen_getjmptarget(gen);
  bool needclose = klgen_stmtblock(gen, whileast->block);
  if (needclose)
      klgen_emit(gen, klinst_close(stktop), klgen_astposition(whileast));
  KlCPC continuepos = klgen_getjmptarget(gen);
  KlCodeVal truelist = klgen_exprbool(gen, whileast->cond, true);
  if (klcodeval_isconstant(truelist)) {
    if (klcodeval_istrue(truelist)) {
      bool cond_has_side_effect = klgen_currentpc(gen) != continuepos;
      int offset = (int)loopbeginpos - (int)klgen_currentpc(gen) - 1;
      if (!klinst_inrange(offset, 24))
        klgen_error_fatal(gen, "jump too far, can not generate code");
      klgen_emit(gen, klinst_jmp(offset), klgen_astposition(whileast->cond));
      klgen_jumpto(gen, cjmplist, cond_has_side_effect ? continuepos : loopbeginpos);
      klgen_jumpto(gen, bjmplist, klgen_getjmptarget(gen));
    } else {
      klgen_jumpto(gen, cjmplist, continuepos);
      klgen_jumpto(gen, bjmplist, klgen_getjmptarget(gen));
    }
  } else {
    klgen_jumpto(gen, truelist, loopbeginpos);
    klgen_jumpto(gen, cjmplist, continuepos);
    klgen_jumpto(gen, bjmplist, klgen_getjmptarget(gen));
  }
  gen->jmpinfo.breaklist = prev_bjmplist;
  gen->jmpinfo.continuelist = prev_cjmplist;
  gen->jmpinfo.break_scope = prev_bscope;
  gen->jmpinfo.continue_scope = prev_cscope;
}

static void gen_repeat(KlGenUnit* gen, KlAstStmtRepeat* repeatast) {
  KlCodeVal bjmplist = klcodeval_none();
  KlCodeVal cjmplist = klcodeval_none();
  KlCodeVal* prev_bjmplist = gen->jmpinfo.breaklist;
  KlCodeVal* prev_cjmplist = gen->jmpinfo.continuelist;
  KlSymTbl* prev_bscope = gen->jmpinfo.break_scope;
  KlSymTbl* prev_cscope = gen->jmpinfo.continue_scope;
  gen->jmpinfo.breaklist = &bjmplist;
  gen->jmpinfo.continuelist = &cjmplist;
  gen->jmpinfo.break_scope = gen->symtbl;
  gen->jmpinfo.continue_scope = gen->symtbl;

  KlCStkId stktop = klgen_stacktop(gen);
  KlCPC loopbeginpos = klgen_getjmptarget(gen);
  bool needclose = klgen_stmtblock(gen, repeatast->block);
  if (needclose)
      klgen_emit(gen, klinst_close(stktop), klgen_astposition(repeatast));
  KlCPC continuepos = klgen_getjmptarget(gen);
  KlCodeVal falselist = klgen_exprbool(gen, repeatast->cond, false);
  if (klcodeval_isconstant(falselist)) {
    if (klcodeval_istrue(falselist)) {
      klgen_jumpto(gen, cjmplist, continuepos);
      klgen_jumpto(gen, bjmplist, klgen_getjmptarget(gen));
    } else {
      bool cond_has_side_effect = klgen_currentpc(gen) != continuepos;
      int offset = (int)loopbeginpos - (int)klgen_currentpc(gen) - 1;
      if (!klinst_inrange(offset, 24))
        klgen_error_fatal(gen, "jump too far, can not generate code");
      klgen_emit(gen, klinst_jmp(offset), klgen_astposition(repeatast->cond));
      klgen_jumpto(gen, cjmplist, cond_has_side_effect ? continuepos : loopbeginpos);
      klgen_jumpto(gen, bjmplist, klgen_getjmptarget(gen));
    }
  } else {
    klgen_jumpto(gen, falselist, loopbeginpos);
    klgen_jumpto(gen, cjmplist, continuepos);
    klgen_jumpto(gen, bjmplist, klgen_getjmptarget(gen));
  }
  gen->jmpinfo.breaklist = prev_bjmplist;
  gen->jmpinfo.continuelist = prev_cjmplist;
  gen->jmpinfo.break_scope = prev_bscope;
  gen->jmpinfo.continue_scope = prev_cscope;
}

static void gen_break(KlGenUnit* gen, KlAstStmtBreak* breakast) {
  if (kl_unlikely(!gen->jmpinfo.breaklist)) {
    klgen_error(gen, klast_begin(breakast), klast_end(breakast), "break statement is not allowed here");
    return;
  }
  KlCPC closebase;
  KlInstruction breakinst;
  if (needclose(gen, gen->jmpinfo.break_scope, &closebase)) {
    breakinst = klinst_closejmp(closebase, 0);
  } else {
    breakinst = klinst_jmp(0);
  }
  klgen_mergejmplist_maynone(gen, gen->jmpinfo.breaklist,
                             klcodeval_jmplist(klgen_emit(gen, breakinst, klgen_astposition(breakast))));
}

static void gen_continue(KlGenUnit* gen, KlAstStmtContinue* continueast) {
  if (kl_unlikely(!gen->jmpinfo.continuelist)) {
    klgen_error(gen, klast_begin(continueast), klast_end(continueast), "continue statement is not allowed here");
    return;
  }
  KlCPC closebase;
  KlInstruction continueinst;
  if (needclose(gen, gen->jmpinfo.continue_scope, &closebase)) {
    continueinst = klinst_closejmp(closebase, 0);
  } else {
    continueinst = klinst_jmp(0);
  }
  klgen_mergejmplist_maynone(gen, gen->jmpinfo.continuelist,
                         klcodeval_jmplist(klgen_emit(gen, continueinst, klgen_astposition(continueast))));
}

static void gen_return(KlGenUnit* gen, KlAstStmtReturn* returnast) {
  KlAstExprList* res = returnast->retvals;
  KlCStkId stktop = klgen_stacktop(gen);
  if (res->nexpr == 1) {
    KlCodeVal retval;
    size_t nres = klgen_expr_inreturn(gen, res->exprs[0], &retval);
    kl_assert(retval.kind == KLVAL_STACK, "");
    if (needclose(gen, gen->reftbl, NULL))
      klgen_emit(gen, klinst_close(0), klgen_astposition(returnast));
    KlInstruction returninst = nres == 0             ? klinst_return0() :
                               nres == KLINST_VARRES ? klinst_return(retval.index, nres)
                                                     : klinst_return1(retval.index);
    klgen_emit(gen, returninst, klgen_astposition(returnast));
  } else {
    KlCStkId argbase = klgen_stacktop(gen);
    size_t nres = klgen_passargs(gen, res);
    if (needclose(gen, gen->reftbl, NULL))
      klgen_emit(gen, klinst_close(0), klgen_astposition(returnast));
    KlInstruction returninst = nres == 0 ? klinst_return0() :
                               nres == 1 ? klinst_return1(argbase)
                                         : klinst_return(argbase, nres);
    klgen_emit(gen, returninst, klgen_astposition(returnast));
  }
  klgen_stackfree(gen, stktop);
}

static void gen_ifor(KlGenUnit* gen, KlAstStmtIFor* iforast) {
  /* compute iteration information(begin, end, step) */
  KlCStkId forbase = klgen_stacktop(gen);
  klgen_expr_evaluated_to_noconst(gen, iforast->begin, forbase);
  // KlCodeVal begin = klcodeval_stack(forbase);
  klgen_expr_evaluated_to_noconst(gen, iforast->end, forbase + 1);
  if (iforast->step) {
    klgen_expr_evaluated_to_noconst(gen, iforast->step, forbase + 2);
  } else {
    klgen_emitloadnils(gen, forbase + 2, 1, klgen_position(klast_end(iforast->end), klast_end(iforast->end)));
    klgen_stackalloc1(gen);
  }
  kl_assert(klgen_stacktop(gen) == forbase + 3, "");

  /* now enable statement 'break' and 'continue' */
  KlCodeVal bjmplist = klcodeval_none();
  KlCodeVal cjmplist = klcodeval_none();
  KlCodeVal* prev_bjmplist = gen->jmpinfo.breaklist;
  KlCodeVal* prev_cjmplist = gen->jmpinfo.continuelist;
  KlSymTbl* prev_bscope = gen->jmpinfo.break_scope;
  KlSymTbl* prev_cscope = gen->jmpinfo.continue_scope;
  gen->jmpinfo.breaklist = &bjmplist;
  gen->jmpinfo.continuelist = &cjmplist;
  gen->jmpinfo.break_scope = gen->symtbl;
  gen->jmpinfo.continue_scope = gen->symtbl;
  /* statement 'break' and 'continue' are enabled */

  klgen_mergejmplist_maynone(gen, gen->jmpinfo.breaklist,
                             klcodeval_jmplist(klgen_emit(gen, klinst_iforprep(forbase, 0), klgen_astposition(iforast))));
  klgen_pushsymtbl(gen);  /* enter a new scope here */
  KlCPC looppos = klgen_getjmptarget(gen);
  kl_assert(iforast->lval->nexpr == 1, "");

  KlAstExpr* pattern = iforast->lval->exprs[0];
  if ((klast_kind(pattern) == KLAST_EXPR_ID)) {
    klgen_newsymbol(gen, klcast(KlAstIdentifier*, pattern)->id, forbase, klgen_astposition(pattern));
  } else {  /* else is pattern deconstruction */
    klgen_pattern_binding_tostktop(gen, pattern, forbase);
    klgen_pattern_newsymbol(gen, pattern, forbase + 3);
    kl_assert(forbase + 3 + klgen_pattern_count_result(gen, pattern) == klgen_stacktop(gen), "");
  }

  klgen_stmtlist(gen, iforast->block);
  if (gen->symtbl->info.referenced)
      klgen_emit(gen, klinst_close(forbase), klgen_astposition(iforast));
  klgen_popsymtbl(gen);   /* close the scope */
  klgen_jumpto(gen, cjmplist, klgen_getjmptarget(gen));
  klgen_emit(gen, klinst_iforloop(forbase, looppos - klgen_currentpc(gen) - 1), klgen_astposition(iforast));
  klgen_jumpto(gen, bjmplist, klgen_getjmptarget(gen));
  klgen_stackfree(gen, forbase);

  gen->jmpinfo.breaklist = prev_bjmplist;
  gen->jmpinfo.continuelist = prev_cjmplist;
  gen->jmpinfo.break_scope = prev_bscope;
  gen->jmpinfo.continue_scope = prev_cscope;
}

static void gen_vfor(KlGenUnit* gen, KlAstStmtVFor* vforast) {
  KlCStkId forbase = klgen_stacktop(gen);
  KlAstExpr** patterns = vforast->lvals->exprs;
  size_t npattern = vforast->lvals->nexpr;
  KlCodeVal step = klcodeval_integer(npattern);
  klgen_putonstktop(gen, &step, klgen_astposition(vforast));

  /* now enable statement 'break' and 'continue' */
  KlCodeVal bjmplist = klcodeval_none();
  KlCodeVal cjmplist = klcodeval_none();
  KlCodeVal* prev_bjmplist = gen->jmpinfo.breaklist;
  KlCodeVal* prev_cjmplist = gen->jmpinfo.continuelist;
  KlSymTbl* prev_bscope = gen->jmpinfo.break_scope;
  KlSymTbl* prev_cscope = gen->jmpinfo.continue_scope;
  gen->jmpinfo.breaklist = &bjmplist;
  gen->jmpinfo.continuelist = &cjmplist;
  gen->jmpinfo.break_scope = gen->symtbl;
  gen->jmpinfo.continue_scope = gen->symtbl;
  /* statement 'break' and 'continue' are enabled */

  klgen_mergejmplist_maynone(gen, gen->jmpinfo.breaklist,
                             klcodeval_jmplist(klgen_emit(gen, klinst_vforprep(forbase, 0), klgen_astposition(vforast))));
  KlCPC looppos = klgen_getjmptarget(gen);
  klgen_stackalloc1(gen); /* forbase + 0: step, forbase + 1: index */
  klgen_pushsymtbl(gen);  /* begin a new scope */

  klgen_stackalloc(gen, npattern);
  for (size_t i = npattern; i--;) {
    KlAstExpr* pattern = patterns[i];
    KlCStkId valstkid = forbase + 2 + i;
    if (klast_kind(pattern) == KLAST_EXPR_ID) {
      klgen_newsymbol(gen, klcast(KlAstIdentifier*, pattern)->id, valstkid, klgen_astposition(pattern));
    } else {
      KlCStkId newsymbol_base = klgen_stacktop(gen);
      klgen_pattern_binding_tostktop(gen, pattern, valstkid);
      klgen_pattern_newsymbol(gen, pattern, newsymbol_base);
    }
  }

  klgen_stmtlist(gen, vforast->block);
  if (gen->symtbl->info.referenced)
      klgen_emit(gen, klinst_close(forbase), klgen_astposition(vforast));
  klgen_popsymtbl(gen);   /* close the scope */
  klgen_jumpto(gen, cjmplist, klgen_getjmptarget(gen));
  klgen_emit(gen, klinst_vforloop(forbase, looppos - klgen_currentpc(gen) - 1), klgen_astposition(vforast));
  klgen_jumpto(gen, bjmplist, klgen_getjmptarget(gen));
  klgen_stackfree(gen, forbase);

  gen->jmpinfo.breaklist = prev_bjmplist;
  gen->jmpinfo.continuelist = prev_cjmplist;
  gen->jmpinfo.break_scope = prev_bscope;
  gen->jmpinfo.continue_scope = prev_cscope;
}

static void gen_gfor(KlGenUnit* gen, KlAstStmtGFor* gforast) {
  /* compute iterable object */
  KlCStkId forbase = klgen_stacktop(gen);
  KlCStkId iterable = forbase;
  klgen_expr_evaluated_to_noconst(gen, gforast->expr, iterable);
  klgen_stackfree(gen, forbase);

  /* now enable statement 'break' and 'continue' */
  KlCodeVal bjmplist = klcodeval_none();
  KlCodeVal cjmplist = klcodeval_none();
  KlCodeVal* prev_bjmplist = gen->jmpinfo.breaklist;
  KlCodeVal* prev_cjmplist = gen->jmpinfo.continuelist;
  KlSymTbl* prev_bscope = gen->jmpinfo.break_scope;
  KlSymTbl* prev_cscope = gen->jmpinfo.continue_scope;
  gen->jmpinfo.breaklist = &bjmplist;
  gen->jmpinfo.continuelist = &cjmplist;
  gen->jmpinfo.break_scope = gen->symtbl;
  gen->jmpinfo.continue_scope = gen->symtbl;
  /* statement 'break' and 'continue' are enabled */

  klgen_emit(gen, klinst_gforprep(iterable, gforast->lvals->nexpr), klgen_astposition(gforast));
  klgen_mergejmplist_maynone(gen, gen->jmpinfo.breaklist,
                             klcodeval_jmplist(klgen_emit(gen, klinst_falsejmp(iterable + 2, 0), klgen_astposition(gforast))));

  KlCPC looppos = klgen_getjmptarget(gen);
  klgen_pushsymtbl(gen);    /* begin a new scope */
  klgen_stackalloc(gen, 3); /* forbase: iteration function, forbase + 1: static state, forbase + 2: index state */

  KlAstExpr** patterns = gforast->lvals->exprs;
  size_t npattern = gforast->lvals->nexpr;
  klgen_stackalloc(gen, npattern);
  for (size_t i = npattern; i--;) {
    KlAstExpr* pattern = patterns[i];
    KlCStkId valstkid = forbase + 3 + i;
    if (klast_kind(pattern) == KLAST_EXPR_ID) {
      klgen_newsymbol(gen, klcast(KlAstIdentifier*, pattern)->id, valstkid, klgen_astposition(pattern));
    } else {
      KlCStkId newsymbol_base = klgen_stacktop(gen);
      klgen_pattern_binding_tostktop(gen, pattern, valstkid);
      klgen_pattern_newsymbol(gen, pattern, newsymbol_base);
    }
  }

  klgen_stmtlist(gen, gforast->block);
  if (gen->symtbl->info.referenced)
      klgen_emit(gen, klinst_close(forbase), klgen_astposition(gforast));
  klgen_popsymtbl(gen);   /* close the scope */
  klgen_jumpto(gen, cjmplist, klgen_getjmptarget(gen));
  klgen_emit(gen, klinst_gforloop(forbase, npattern + 2), klgen_astposition(gforast));
  klgen_emit(gen, klinst_truejmp(forbase + 2, looppos - klgen_currentpc(gen) - 1), klgen_astposition(gforast));
  klgen_jumpto(gen, bjmplist, klgen_getjmptarget(gen));
  klgen_stackfree(gen, forbase);

  gen->jmpinfo.breaklist = prev_bjmplist;
  gen->jmpinfo.continuelist = prev_cjmplist;
  gen->jmpinfo.break_scope = prev_bscope;
  gen->jmpinfo.continue_scope = prev_cscope;
}

void klgen_stmtlist(KlGenUnit* gen, KlAstStmtList* ast) {
  KlAstStmt** endstmt = ast->stmts + ast->nstmt;
  for (KlAstStmt** pstmt = ast->stmts; pstmt != endstmt; ++pstmt) {
    KlAstStmt* stmt = *pstmt;
    switch (klast_kind(stmt)) {
      case KLAST_STMT_LOCALFUNC: {
        gen_localdef(gen, klcast(KlAstStmtLocalDefinition*, stmt));
        break;
      }
      case KLAST_STMT_LET: {
        gen_let(gen, klcast(KlAstStmtLet*, stmt));
        break;
      }
      case KLAST_STMT_METHOD: {
        gen_method(gen, klcast(KlAstStmtMethod*, stmt));
        break;
      }
      case KLAST_STMT_MATCH: {
        gen_match(gen, klcast(KlAstStmtMatch*, stmt));
        break;
      }
      case KLAST_STMT_ASSIGN: {
        gen_assign(gen, klcast(KlAstStmtAssign*, stmt));
        break;
      }
      case KLAST_STMT_EXPR: {
        KlAstExprList* exprlist = klcast(KlAstStmtExpr*, stmt)->exprlist;
        klgen_exprlist_raw(gen, exprlist->exprs, exprlist->nexpr, 0, klgen_astposition(exprlist));
        break;
      }
      case KLAST_STMT_BLOCK: {
        klgen_stmtblock(gen, klcast(KlAstStmtList*, stmt));
        break;
      }
      case KLAST_STMT_IF: {
        gen_if(gen, klcast(KlAstStmtIf*, stmt));
        break;
      }
      case KLAST_STMT_IFOR: {
        gen_ifor(gen, klcast(KlAstStmtIFor*, stmt));
        break;
      }
      case KLAST_STMT_VFOR: {
        gen_vfor(gen, klcast(KlAstStmtVFor*, stmt));
        break;
      }
      case KLAST_STMT_GFOR: {
        gen_gfor(gen, klcast(KlAstStmtGFor*, stmt));
        break;
      }
      case KLAST_STMT_WHILE: {
        gen_while(gen, klcast(KlAstStmtWhile*, stmt));
        break;
      }
      case KLAST_STMT_REPEAT: {
        gen_repeat(gen, klcast(KlAstStmtRepeat*, stmt));
        break;
      }
      case KLAST_STMT_BREAK: {
        gen_break(gen, klcast(KlAstStmtBreak*, stmt));
        break;
      }
      case KLAST_STMT_CONTINUE: {
        gen_continue(gen, klcast(KlAstStmtContinue*, stmt));
        break;
      }
      case KLAST_STMT_RETURN: {
        gen_return(gen, klcast(KlAstStmtReturn*, stmt));
        break;
      }
      default: {
        kl_assert(false, "control flow should not reach here");
        break;
      }
    }
  }
}
