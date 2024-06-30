#include "include/parse/klparser_stmt.h"
#include "include/error/klerror.h"
#include "include/parse/kllex.h"
#include "include/parse/klparser_expr.h"
#include "include/parse/klparser_utils.h"

static KlAst* klparser_stmtexprandassign(KlParser* parser, KlLex* lex);
static KlAstStmtLocalDefinition* klparser_stmtlocaldefinition(KlParser* parser, KlLex* lex);
static KlAstStmtMethod* klparser_stmtmethod(KlParser* parser, KlLex* lex);
static KlAstStmtMatch* klparser_stmtmatch(KlParser* parser, KlLex* lex);
static KlAstStmtIf* klparser_stmtif(KlParser* parser, KlLex* lex);
static KlAst* klparser_stmtfor(KlParser* parser, KlLex* lex);
static KlAstStmtWhile* klparser_stmtwhile(KlParser* parser, KlLex* lex);
static KlAstStmtRepeat* klparser_stmtrepeat(KlParser* parser, KlLex* lex);
static KlAstStmtReturn* klparser_stmtreturn(KlParser* parser, KlLex* lex);
static inline KlAstStmtBreak* klparser_stmtbreak(KlParser* parser, KlLex* lex);
static inline KlAstStmtContinue* klparser_stmtcontinue(KlParser* parser, KlLex* lex);


KlAstStmtList* klparser_stmtblock(KlParser* parser, KlLex* lex) {
  if (kllex_check(lex, KLTK_LBRACE)) {
    KlFileOffset begin = kllex_tokbegin(lex);
    kllex_next(lex);
    KlAstStmtList* stmtlist = klparser_stmtlist(parser, lex);
    if (stmtlist) klast_setposition(stmtlist, begin, kllex_tokend(lex));
    klparser_match(parser, lex, KLTK_RBRACE);
    return stmtlist;
  } else {
    KlAst* stmt = klparser_stmt(parser, lex);
    klparser_returnifnull(stmt);
    KlAst** stmts = (KlAst**)malloc(1 * sizeof (KlAst*));
    if (kl_unlikely(!stmts)) {
      klast_delete(stmt);
      return klparser_error_oom(parser, lex);
    }
    stmts[0] = stmt;
    KlAstStmtList* stmtlist = klast_stmtlist_create(stmts, 1, stmt->begin, stmt->end);
    klparser_oomifnull(stmtlist);
    return stmtlist;
  }
}

KlAst* klparser_stmt(KlParser* parser, KlLex* lex) {
  switch (kllex_tokkind(lex)) {
    case KLTK_LOCAL: {
      KlAstStmtLocalDefinition* stmtlocalfunc = klparser_stmtlocaldefinition(parser, lex);
      return klast(stmtlocalfunc);
    }
    case KLTK_LET: {
      KlAstStmtLet* stmtlet = klparser_stmtlet(parser, lex);
      return klast(stmtlet);
    }
    case KLTK_METHOD: {
      KlAstStmtMethod* stmtmethod = klparser_stmtmethod(parser, lex);
      return klast(stmtmethod);
    }
    case KLTK_MATCH: {
      KlAstStmtMatch* stmtmatch = klparser_stmtmatch(parser, lex);
      return klast(stmtmatch);
    }
    case KLTK_IF: {
      KlAstStmtIf* stmtif = klparser_stmtif(parser, lex);
      return klast(stmtif);
    }
    case KLTK_REPEAT: {
      KlAstStmtRepeat* stmtrepeat = klparser_stmtrepeat(parser, lex);
      return klast(stmtrepeat);
    }
    case KLTK_WHILE: {
      KlAstStmtWhile* stmtwhile = klparser_stmtwhile(parser, lex);
      return klast(stmtwhile);
    }
    case KLTK_RETURN: {
      KlAstStmtReturn* stmtreturn = klparser_stmtreturn(parser, lex);
      return klast(stmtreturn);
    }
    case KLTK_BREAK: {
      KlAstStmtBreak* stmtbreak = klparser_stmtbreak(parser, lex);
      return klast(stmtbreak);
    }
    case KLTK_CONTINUE: {
      KlAstStmtContinue* stmtcontinue = klparser_stmtcontinue(parser, lex);
      return klast(stmtcontinue);
    }
    case KLTK_FOR: {
      KlAst* stmtfor = klparser_stmtfor(parser, lex);
      return stmtfor;
    }
    default: {
      return klparser_stmtexprandassign(parser, lex);
    }
  }
}

static KlAst* klparser_discoveryforblock(KlParser* parser, KlLex* lex) {
  klparser_match(parser, lex, KLTK_COLON);
  KlAstStmtList* block = klparser_stmtblock(parser, lex);
  if (block) klast_delete(block);
  return NULL;
}

static inline KlAstStmtList* klparser_finishstmtfor(KlParser* parser, KlLex* lex) {
  kllex_trymatch(lex, KLTK_COLON);
  return klparser_stmtblock(parser, lex);
}

static KlAst* klparser_stmtinfor(KlParser* parser, KlLex* lex) {
  KlAstExprList* lvals = klparser_exprlist(parser, lex);
  if (kllex_trymatch(lex, KLTK_ASSIGN)) { /* stmtfor -> for i = n, m, s */
    KlAstExprList* exprlist = klparser_exprlist(parser, lex);
    if (kl_unlikely(!exprlist)) {
      if (lvals) klast_delete(lvals);
      return klparser_discoveryforblock(parser, lex);
    }
    KlAstStmtList* block = klparser_finishstmtfor(parser, lex);
    if (kl_unlikely(!block)) {
      if (lvals) klast_delete(lvals);
      klast_delete(exprlist);
      return NULL;
    }
    if (kl_unlikely(!lvals)) {
      klast_delete(exprlist);
      klast_delete(block);
      return NULL;
    }
    if (kl_unlikely(lvals->nexpr != 1))
      klparser_error(parser, kllex_inputstream(lex), klast_begin(lvals), klast_end(lvals), "integer loop requires only one iteration variable");
    size_t nexpr = exprlist->nexpr;
    if (kl_unlikely(nexpr != 3 && nexpr != 2)) {
      klparser_error(parser, kllex_inputstream(lex), klast_begin(exprlist), klast_end(exprlist), "expected 2 or 3 expressions here");
      klast_delete(lvals);
      klast_delete(block);
      klast_delete(exprlist);
      return NULL;
    }
    KlAst** exprs = exprlist->exprs;
    KlAstStmtIFor* ifor = klast_stmtifor_create(lvals, exprs[0], exprs[1], nexpr == 3 ? exprs[2] : NULL, block, KLPARSER_ERROR_PH_FILEPOS, klast_end(block));
    klast_exprlist_shallow_replace(exprlist, NULL, 0);
    klast_delete(exprlist);
    klparser_oomifnull(ifor);
    return klast(ifor);
  }
  /* stmtfor -> for a, b, ... in expr */
  klparser_match(parser, lex, KLTK_IN);
  KlAst* iterable = klparser_expr(parser, lex);
  if (kl_unlikely(!iterable)) {
    if (lvals) klast_delete(lvals);
    return klparser_discoveryforblock(parser, lex);
  }
  KlAstStmtList* block = klparser_finishstmtfor(parser, lex);
  if (kl_unlikely(!block)) {
    if (lvals) klast_delete(lvals);
    klast_delete(iterable);
    return NULL;
  }
  if (kl_unlikely(!lvals)) {
    klast_delete(iterable);
    klast_delete(block);
    return NULL;
  }
  kl_assert(lvals->nexpr != 0, "");
  if (klast_kind(iterable) == KLAST_EXPR_VARARG) {
    /* is variable argument for loop : stmtfor -> for a, b, ... in ... */
    klast_delete(iterable); /* 'iterable' is no longer needed */
    KlAstStmtVFor* vfor = klast_stmtvfor_create(lvals, block, KLPARSER_ERROR_PH_FILEPOS, klast_end(block));
    klparser_oomifnull(vfor);
    return klast(vfor);
  } else {  /* generic for loop : stmtfor -> for a, b, ... in expr */
    KlAstStmtGFor* gfor = klast_stmtgfor_create(lvals, iterable, block, KLPARSER_ERROR_PH_FILEPOS, klast_end(block));
    klparser_oomifnull(gfor);
    return klast(gfor);
  }
}

KlAstStmtLet* klparser_stmtlet(KlParser* parser, KlLex* lex) {
  kl_assert(kllex_check(lex, KLTK_LET), "expected 'let'");
  KlFileOffset begin = kllex_tokbegin(lex);
  kllex_next(lex);
  KlAstExprList* lvals = klparser_exprlist(parser, lex);
  klparser_match(parser, lex, KLTK_ASSIGN);
  KlAstExprList* exprlist = klparser_exprlist(parser, lex);
  if (kl_unlikely(!exprlist || !lvals)) {
    if (lvals) klast_delete(lvals);
    if (exprlist) klast_delete(exprlist);
    return NULL;
  }
  KlAstStmtLet* stmtlet = klast_stmtlet_create(lvals, exprlist, begin, klast_end(exprlist));
  klparser_oomifnull(stmtlet);
  return stmtlet;
}

static KlAstStmtMethod* klparser_stmtmethod(KlParser* parser, KlLex* lex) {
  kl_assert(kllex_check(lex, KLTK_METHOD), "expected 'method'");
  KlFileOffset begin = kllex_tokbegin(lex);
  kllex_next(lex);
  KlAst* lval = klparser_expr(parser, lex);
  klparser_match(parser, lex, KLTK_ASSIGN);
  KlAst* rval = klparser_expr(parser, lex);
  if (kl_unlikely(!lval || !rval)) {
    if (lval) klast_delete(lval);
    if (rval) klast_delete(rval);
    return NULL;
  }
  if (klast_kind(lval) != KLAST_EXPR_DOT) {
    klparser_error(parser, kllex_inputstream(lex), klast_begin(lval), klast_end(lval), "should be a '.' expression");
    klast_delete(lval);
    klast_delete(rval);
    return NULL;
  }
  KlAstStmtMethod* stmtmethod = klast_stmtmethod_create(klcast(KlAstDot*, lval), rval, begin, klast_end(rval));
  klparser_oomifnull(stmtmethod);
  return stmtmethod;
}

static KlAstStmtMatch* klparser_stmtmatch(KlParser* parser, KlLex* lex) {
  kl_assert(kllex_check(lex, KLTK_MATCH), "expected 'match'");
  KlFileOffset begin = kllex_tokbegin(lex);
  kllex_next(lex);
  KlAst* matchobj = klparser_expr(parser, lex);
  klparser_match(parser, lex, KLTK_COLON);
  klparser_match(parser, lex, KLTK_LBRACE);
  KArray patterns;
  KArray stmts;
  if (kl_unlikely(!karray_init(&patterns) || !karray_init(&stmts))) {
    karray_destroy(&patterns);
    karray_destroy(&stmts);
    if (matchobj) klast_delete(matchobj);
    return klparser_error_oom(parser, lex);
  }
  do {
    KlAst* pattern = klparser_expr(parser, lex);
    klparser_match(parser, lex, KLTK_COLON);
    KlAstStmtList* stmtlist = klparser_stmtblock(parser, lex);
    kllex_trymatch(lex, KLTK_SEMI);
    if (kl_unlikely(!pattern || !stmtlist)) {
      if (pattern) klast_delete(pattern);
      if (stmtlist) klast_delete(stmtlist);
    } else {
      klparser_karr_pushast(&patterns, pattern);
      klparser_karr_pushast(&stmts, stmtlist);
    }
  } while (klparser_exprbegin(lex));
  klparser_match(parser, lex, KLTK_RBRACE);
  if (kl_unlikely(karray_size(&patterns) != karray_size(&stmts) ||
                  karray_size(&patterns) == 0 ||
                  !matchobj)) {
    klparser_destroy_astarray(&patterns);
    klparser_destroy_astarray(&stmts);
    if (matchobj) klast_delete(matchobj);
    return NULL;
  }
  karray_shrink(&patterns);
  karray_shrink(&stmts);
  size_t npattern = karray_size(&patterns);
  KlAst** patterns_stolen = klcast(KlAst**, karray_steal(&patterns));
  KlAstStmtList** stmts_stolen = klcast(KlAstStmtList**, karray_steal(&stmts));
  KlAstStmtMatch* stmtmatch = klast_stmtmatch_create(matchobj, patterns_stolen, stmts_stolen, npattern, begin, klast_end(stmts_stolen[npattern - 1]));
  klparser_oomifnull(stmtmatch);
  return stmtmatch;
}

static KlAst* klparser_stmtexprandassign(KlParser* parser, KlLex* lex) {
  KlAstExprList* maybelvals = klparser_exprlist(parser, lex);
  if (kllex_trymatch(lex, KLTK_ASSIGN)) { /* is assignment */
    KlAstExprList* rvals = klparser_exprlist(parser, lex);
    if (kl_unlikely(!rvals || !maybelvals)) {
      if (rvals) klast_delete(rvals);
      if (maybelvals) klast_delete(maybelvals);
      return NULL;
    }
    KlAstStmtAssign* stmtassign = klast_stmtassign_create(maybelvals, rvals, klast_begin(maybelvals), klast_end(rvals));
    klparser_oomifnull(stmtassign);
    return klast(stmtassign);
  } else {
    klparser_returnifnull(maybelvals);
    /* just an expression list statement */
    KlAstStmtExpr* stmtexpr = klast_stmtexpr_create(maybelvals, klast_begin(maybelvals), klast_end(maybelvals));
    klparser_oomifnull(stmtexpr);
    return klast(stmtexpr);
  }
}

static KlAstStmtLocalDefinition* klparser_stmtlocaldefinition(KlParser* parser, KlLex* lex) {
  kl_assert(kllex_check(lex, KLTK_LOCAL), "expected 'local'");
  KlFileOffset begin = kllex_tokbegin(lex);
  kllex_next(lex);
  if (kl_unlikely(!klparser_check(parser, lex, KLTK_ID)))
    return NULL;
  KlFileOffset idbegin = kllex_tokbegin(lex);
  KlFileOffset idend = kllex_tokend(lex);
  KlStrDesc funcid = lex->tok.string;
  kllex_next(lex);
  KlAst* expr = klparser_expr(parser, lex);
  klparser_returnifnull(expr);
  if (kl_unlikely(klast_kind(expr) != KLAST_EXPR_FUNC && klast_kind(expr) != KLAST_EXPR_CLASS)) {
    klparser_error(parser, kllex_inputstream(lex), klast_begin(expr), klast_end(expr), "expected function or class");
    klast_delete(expr);
    return NULL;
  }
  KlAstStmtLocalDefinition* stmtlocalfunc = klast_stmtlocaldef_create(funcid, idbegin, idend, expr, begin, klast_end(expr));
  klparser_oomifnull(stmtlocalfunc);
  return stmtlocalfunc;
}

static KlAstStmtIf* klparser_stmtif(KlParser* parser, KlLex* lex) {
  kl_assert(kllex_check(lex, KLTK_IF), "expected 'if'");
  KlFileOffset begin = kllex_tokbegin(lex);
  kllex_next(lex);
  KlAst* cond = klparser_expr(parser, lex);
  kllex_trymatch(lex, KLTK_COLON);
  KlAstStmtList* then_block = NULL;
  then_block = klparser_stmtblock(parser, lex);
  KlAstStmtList* else_block = NULL;
  if (kllex_trymatch(lex, KLTK_ELSE)) {     /* else block */
    kllex_trymatch(lex, KLTK_COLON);
    else_block = klparser_stmtblock(parser, lex);
  }
  if (kl_unlikely(!then_block || !cond)) {
    if (cond) klast_delete(cond);
    if (then_block) klast_delete(then_block);
    return NULL;
  }
  KlAstStmtIf* stmtif = klast_stmtif_create(cond, then_block, else_block, begin, else_block ? klast_end(else_block): klast_end(then_block));
  klparser_oomifnull(stmtif);
  return stmtif;
}

static KlAst* klparser_stmtfor(KlParser* parser, KlLex* lex) {
  kl_assert(kllex_check(lex, KLTK_FOR), "expected '('");
  KlFileOffset begin = kllex_tokbegin(lex);
  kllex_next(lex);
  KlAst* stmtfor = klparser_stmtinfor(parser, lex);
  klparser_returnifnull(stmtfor);
  klast_setposition(klcast(KlAst*, stmtfor), begin, stmtfor->end);
  return stmtfor;
}

static KlAstStmtWhile* klparser_stmtwhile(KlParser* parser, KlLex* lex) {
  kl_assert(kllex_check(lex, KLTK_WHILE), "expected 'while'");
  KlFileOffset begin = kllex_tokbegin(lex);
  kllex_next(lex);
  KlAst* cond = klparser_expr(parser, lex);
  klparser_match(parser, lex, KLTK_COLON);
  KlAstStmtList* block = klparser_stmtblock(parser, lex);
  if (kl_unlikely(!cond || !block)) {
    if (cond) klast_delete(cond);
    if (block) klast_delete(block);
    return NULL;
  }
  KlAstStmtWhile* stmtwhile = klast_stmtwhile_create(cond, block, begin, klast_end(block));
  klparser_oomifnull(stmtwhile);
  return stmtwhile;
}

KlAstStmtList* klparser_stmtlist(KlParser* parser, KlLex* lex) {
  KArray stmts;
  if (kl_unlikely(!karray_init(&stmts)))
    return klparser_error_oom(parser, lex);

  while (klparser_stmtbegin(lex)) {
    KlAst* stmt = klparser_stmt(parser, lex);
    if (kl_unlikely(!stmt)) continue;
    if (kl_unlikely(!karray_push_back(&stmts, stmt))) {
      klast_delete(stmt);
      klparser_error_oom(parser, lex);
    }
    kllex_trymatch(lex, KLTK_SEMI);
  }

  karray_shrink(&stmts);
  size_t nstmt = karray_size(&stmts);
  KlAstStmtList* stmtlist = klast_stmtlist_create((KlAst**)karray_steal(&stmts), nstmt, KLPARSER_ERROR_PH_FILEPOS, KLPARSER_ERROR_PH_FILEPOS);
  klparser_oomifnull(stmtlist);
  return stmtlist;
}

static KlAst* klparser_stmtrepeat_defaultcondition(KlParser* parser, KlLex* lex, KlFileOffset begin, KlFileOffset end) {
  KlAst* con = klast(klast_constant_create_boolean(KLC_FALSE, begin, end));
  klparser_oomifnull(con);
  return con;
}

static KlAstStmtRepeat* klparser_stmtrepeat(KlParser* parser, KlLex* lex) {
  kl_assert(kllex_check(lex, KLTK_REPEAT), "expected 'repeat'");
  KlFileOffset begin = kllex_tokbegin(lex);
  kllex_next(lex);
  kllex_trymatch(lex, KLTK_COLON);
  KlAstStmtList* block = klparser_stmtblock(parser, lex);
  KlAst* cond = kllex_trymatch(lex, KLTK_UNTIL)
              ? klparser_expr(parser, lex)
              : klparser_stmtrepeat_defaultcondition(parser, lex, klast_end(block), klast_end(block));
  if (kl_unlikely(!cond || !block)) {
    if (cond) klast_delete(cond);
    if (block) klast_delete(block);
    return NULL;
  }
  KlAstStmtRepeat* stmtrepeat = klast_stmtrepeat_create(block, cond, begin, cond->end);
  klparser_oomifnull(stmtrepeat);
  return stmtrepeat;
}

static KlAstStmtReturn* klparser_stmtreturn(KlParser* parser, KlLex* lex) {
  kl_assert(kllex_check(lex, KLTK_RETURN), "expected 'return'");
  KlFileOffset begin = kllex_tokbegin(lex);
  kllex_next(lex);
  KlAstExprList* results = klparser_exprlist_mayempty(parser, lex);
  klparser_returnifnull(results);
  KlAstStmtReturn* stmtreturn = klast_stmtreturn_create(results, begin, klast_end(results));
  klparser_oomifnull(stmtreturn);
  return stmtreturn;
}
