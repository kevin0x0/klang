#include "include/parse/klparser_comprehension.h"
#include "include/parse/klparser_expr.h"
#include "include/parse/klparser_stmt.h"
#include "include/parse/klparser_utils.h"
#include "deps/k/include/array/karray.h"

static KlAst* parse_new_block(KlParser* parser, KlLex* lex, KlAstExprList* lvals, KlAstStmt* inner_stmt) {
  if (kllex_trymatch(lex, KLTK_ASSIGN)) { /* i = n, m, s */
    KlAstExprList* exprlist = klparser_exprlist(parser, lex);
    kllex_trymatch(lex, KLTK_SEMI);
    if (kl_unlikely(!exprlist)) {
      klast_delete(exprlist);
      klast_delete(lvals);
      klast_delete(inner_stmt);
      return NULL;
    }
    size_t nelem = exprlist->nexpr;
    if (kl_unlikely(nelem != 3 && nelem != 2)) {
      klparser_error(parser, kllex_inputstream(lex), klast_begin(exprlist), klast_end(exprlist), "expected 2 or 3 expressions here");
      klast_delete(exprlist);
      klast_delete(inner_stmt);
      klast_delete(lvals);
      return NULL;
    }
    if (kl_unlikely(lvals->nexpr != 1)) {
      klparser_error(parser, kllex_inputstream(lex), klast_begin(lvals), klast_end(lvals), "integer loop requires only one iteration variable");
      klast_delete(exprlist);
      klast_delete(inner_stmt);
      klast_delete(lvals);
      return NULL;
    }
    KlAstStmtList* block = klparser_comprehension(parser, lex, inner_stmt);
    if (kl_unlikely(!block)) {
      klast_delete(exprlist);
      klast_delete(lvals);
      return NULL;
    }
    KlAstExpr** exprs = exprlist->exprs;
    KlAstStmtIFor* ifor = klast_stmtifor_create(lvals, exprs[0], exprs[1], nelem == 3 ? exprs[2] : NULL, block, klast_begin(lvals), klast_end(block));
    klast_exprlist_shallow_replace(exprlist, NULL, 0);
    klast_delete(exprlist);
    klparser_oomifnull(ifor);
    return klast(ifor);
  }
  /* a, b, ... in expr */
  klparser_match(parser, lex, KLTK_IN);
  KlAstExpr* iterable = klparser_expr(parser, lex);
  kllex_trymatch(lex, KLTK_SEMI);
  if (kl_unlikely(!iterable)) {
    klast_delete(inner_stmt);
    klast_delete(lvals);
    return NULL;
  }
  KlAstStmtList* block = klparser_comprehension(parser, lex, inner_stmt);
  if (kl_unlikely(!block)) {
    klast_delete(lvals);
    klast_delete(iterable);
    return NULL;
  }
  if (klast_kind(iterable) == KLAST_EXPR_VARARG) {  /* a, b, ... in ... */
    klast_delete(iterable); /* 'iterable' is no longer needed */
    KlAstStmtVFor* vfor = klast_stmtvfor_create(lvals, block, klast_begin(lvals), klast_end(block));
    klparser_oomifnull(vfor);
    return klast(vfor);
  } else {  /* a, b, ... in expr */
    KlAstStmtGFor* gfor = klast_stmtgfor_create(lvals, iterable, block, klast_begin(lvals), klast_end(block));
    klparser_oomifnull(gfor);
    return klast(gfor);
  }
}

KlAstStmtList* klparser_comprehension(KlParser* parser, KlLex* lex, KlAstStmt* inner_stmt) {
  KArray stmtarr;
  if (kl_unlikely(!karray_init(&stmtarr)))
    return klparser_error_oom(parser, lex);
  while (true) {
    if (klparser_exprbegin(lex)) {
      KlAstExprList* exprlist = klparser_exprlist(parser, lex);
      if (kl_unlikely(!exprlist)) {
        kllex_trymatch(lex, KLTK_SEMI);
        continue;
      }
      if (kllex_check(lex, KLTK_IN) || kllex_check(lex, KLTK_ASSIGN)) {
        KlAst* stmt = parse_new_block(parser, lex, exprlist, inner_stmt);
        if (kl_unlikely(!stmt)) {
          /* 'exprlist' is deleted in klparser_comprehensionfor() */
          klparser_destroy_astarray(&stmtarr);
          return NULL;
        }
        if (kl_unlikely(!karray_push_back(&stmtarr, stmt))) {
          klast_delete(stmt);
          klparser_destroy_astarray(&stmtarr);
          return klparser_error_oom(parser, lex);
        }
      } else {
        kllex_trymatch(lex, KLTK_SEMI);
        KlAstStmtList* block = klparser_comprehension(parser, lex, inner_stmt);
        if (kl_unlikely(!block)) {
          klast_delete(exprlist);
          klparser_destroy_astarray(&stmtarr);
          return NULL;
        }
        if (kl_unlikely(exprlist->nexpr != 1)) {
          klparser_error(parser, kllex_inputstream(lex), klast_begin(exprlist), klast_end(exprlist),
                         "expected a single expression as condition, not an expression list");
        }
        KlAstExpr* expr = klast_exprlist_stealfirst_and_destroy(exprlist);
        KlAstStmtIf* stmtif = klast_stmtif_create(expr, block, NULL, klast_begin(expr), klast_end(expr));
        if (kl_unlikely(!stmtif)) {
          klparser_destroy_astarray(&stmtarr);
          return NULL;
        }
        if (kl_unlikely(!karray_push_back(&stmtarr, stmtif))) {
          klast_delete(klast(stmtif));
          klparser_destroy_astarray(&stmtarr);
          return klparser_error_oom(parser, lex);
        }
      }
      kl_assert(karray_size(&stmtarr) != 0, "");
      karray_shrink(&stmtarr);
      size_t nstmt = karray_size(&stmtarr);
      KlAstStmt** stmts = (KlAstStmt**)karray_steal(&stmtarr);
      KlAstStmtList* stmtlist = klast_stmtlist_create(stmts, nstmt, klast_begin(stmts[0]), klast_end(stmts[nstmt - 1]));
      klparser_oomifnull(stmtlist);
      return stmtlist;
    } else if (kllex_check(lex, KLTK_LET)) {
      KlAstStmtLet* stmtlet = klparser_stmtlet(parser, lex);
      if (kl_unlikely(!stmtlet)) continue;
      if (kl_unlikely(!karray_push_back(&stmtarr, stmtlet))) {
        klast_delete(stmtlet);
        klparser_error_oom(parser, lex);
      }
      kllex_trymatch(lex, KLTK_SEMI);
    } else {
      KlFileOffset begin = karray_size(&stmtarr) == 0 ? kllex_tokbegin(lex) : klast_begin(karray_access(&stmtarr, 0));
      KlFileOffset end = karray_size(&stmtarr) == 0 ? kllex_tokbegin(lex) : klast_end(karray_access(&stmtarr, karray_size(&stmtarr) - 1));
      if (kl_unlikely(!karray_push_back(&stmtarr, inner_stmt))) {
        klparser_destroy_astarray(&stmtarr);
        klast_delete(inner_stmt);
        return klparser_error_oom(parser, lex);
      }
      karray_shrink(&stmtarr);
      size_t nstmt = karray_size(&stmtarr);
      KlAstStmt** stmts = (KlAstStmt**)karray_steal(&stmtarr);
      KlAstStmtList* stmtlist = klast_stmtlist_create(stmts, nstmt, begin, end);
      klparser_oomifnull(stmtlist);
      return stmtlist;
    }
  }
}
