#include "include/parse/klparser_expr.h"
#include "include/ast/klast.h"
#include "include/error/klerror.h"
#include "include/misc/klutils.h"
#include "include/parse/kllex.h"
#include "include/parse/klparser_stmt.h"
#include "include/parse/klparser_comprehension.h"
#include "include/parse/klcfdarr.h"
#include "include/parse/klparser_error.h"
#include "include/parse/klparser_utils.h"
#include "deps/k/include/array/karray.h"
#include "include/parse/kltokens.h"


static KlAstExprList* klparser_correctfuncparams(KlParser* parser, KlLex* lex, KlAstExpr* expr, bool* vararg);
static KlAstStmtList* klparser_darrowfuncbody(KlParser* parser, KlLex* lex);
static KlAstStmtList* klparser_arrowfuncbody(KlParser* parser, KlLex* lex);
/* parse all syntactical structure that begin with '{"(class, map, map comprehension, coroutine comprehension) */
static KlAstExpr* klparser_exprbrace(KlParser* parser, KlLex* lex);
static KlAstExpr* klparser_exprbrace_inner(KlParser* parser, KlLex* lex);
static KlAstExpr* klparser_finishmap(KlParser* parser, KlLex* lex, KlAstExpr* firstkey, KlAstExpr* firstval);
static KlAstMapComprehension* klparser_finishmapcomprehension(KlParser* parser, KlLex* lex, KlAstExpr** keys, KlAstExpr** vals, size_t npair);
static KlAstClass* klparser_finishclass(KlParser* parser, KlLex* lex, KlStrDesc id, KlAstExpr* expr, bool preparsed);
static void klparser_locallist(KlParser* parser, KlLex* lex, KlCfdArray* fields, KArray* vals);
static void klparser_sharedlist(KlParser* parser, KlLex* lex, KlCfdArray* fields, KArray* vals);
static void klparser_classmethod(KlParser* parser, KlLex* lex, KlCfdArray* fields, KArray* vals);
static KlAstExpr* klparser_array(KlParser* parser, KlLex* lex);
static KlAstExpr* klparser_dotchain(KlParser* parser, KlLex* lex);
static KlAstNew* klparser_exprnew(KlParser* parser, KlLex* lex);
static KlAstWhere* klparser_exprfinishwhere(KlParser* parser, KlLex* lex, KlAstExpr* expr);
static KlAstMatch* klparser_exprmatch(KlParser* parser, KlLex* lex);
static KlAstExpr* klparser_exprunit(KlParser* parser, KlLex* lex);
static KlAstExpr* klparser_exprpost(KlParser* parser, KlLex* lex);
static KlAstExpr* klparser_exprpre(KlParser* parser, KlLex* lex);
static KlAstExpr* klparser_exprbin(KlParser* parser, KlLex* lex, int prio);
static KlAstTuple* klparser_emptytuple(KlParser* parser, KlLex* lex, KlFileOffset begin, KlFileOffset end);
static KlAstExpr** klparser_finishcommasepexpr(KlParser* parser, KlLex* lex, KlAstExpr* expr, size_t* pnelem);


KlAstExpr* klparser_expr(KlParser* parser, KlLex* lex) {
  switch (kllex_tokkind(lex)) {
    case KLTK_YIELD: {
      KlFileOffset begin = kllex_tokbegin(lex);
      kllex_next(lex);
      KlAstExprList* exprlist = klparser_exprlist_mayempty(parser, lex);
      klparser_returnifnull(exprlist);
      KlAstYield* yieldexpr = klast_yield_create(exprlist, begin, klast_end(exprlist));
      klparser_oomifnull(yieldexpr);
      return klcast(KlAstExpr*, yieldexpr);
    }
    case KLTK_ASYNC: {
      KlFileOffset begin = kllex_tokbegin(lex);
      kllex_next(lex);
      KlAstExpr* expr = klparser_expr(parser, lex);
      klparser_returnifnull(expr);
      KlAstAsync* asyncexpr = klast_async_create(expr, begin, klast_end(expr));
      klparser_oomifnull(asyncexpr);
      return klcast(KlAstExpr*, asyncexpr);
    }
    case KLTK_ARROW:
    case KLTK_DARROW: {
      KlFileOffset begin = kllex_tokbegin(lex);
      KlAstStmtList* block = kllex_check(lex, KLTK_DARROW)
                           ? klparser_darrowfuncbody(parser, lex)
                           : klparser_arrowfuncbody(parser, lex);
      klparser_returnifnull(block);
      KlAstExprList* params = klparser_emptyexprlist(parser, lex, begin, begin);
      if (kl_unlikely(!params)) {
        klast_delete(block);
        return NULL;
      }
      KlAstFunc* func = klast_func_create(block, params, false, begin, klast_end(block));
      klparser_oomifnull(func);
      return klcast(KlAstExpr*, func);
    }
    case KLTK_CASE: {
      return klcast(KlAstExpr*, klparser_exprmatch(parser, lex));
    }
    default: {
      break;
    }
  }
  KlAstExpr* expr = klparser_exprbin(parser, lex, 0);
  klparser_returnifnull(expr);
  switch (kllex_tokkind(lex)) {
    case KLTK_ARROW:
    case KLTK_DARROW: {
      KlAstStmtList* block = kllex_check(lex, KLTK_DARROW)
                           ? klparser_darrowfuncbody(parser, lex)
                           : klparser_arrowfuncbody(parser, lex);
      if (kl_unlikely(!block)) {
        klast_delete(expr);
        return NULL;
      }
      bool vararg;
      KlAstExprList* params = klparser_correctfuncparams(parser, lex, expr, &vararg);
      if (kl_unlikely(!params)) {
        klast_delete(block);
        return NULL;
      }
      KlAstFunc* func = klast_func_create(block, params, vararg, klast_begin(params), klast_end(block));
      klparser_oomifnull(func);
      return klcast(KlAstExpr*, func);
    }
    case KLTK_WALRUS: {
      kllex_next(lex);
      KlAstExpr* rval = klparser_expr(parser, lex);
      if (kl_unlikely(!rval)) {
        klast_delete(expr);
        return NULL;
      }
      KlAstWalrus* walrus = klast_walrus_create(expr, rval, klast_begin(expr), klast_end(rval));
      klparser_oomifnull(walrus);
      return klcast(KlAstExpr*, walrus);
    }
    case KLTK_WHERE: {
      return klcast(KlAstExpr*, klparser_exprfinishwhere(parser, lex, expr));
    }
    case KLTK_APPEND: {
      kllex_next(lex);
      KlAstExprList* exprlist = klparser_exprlist(parser, lex);
      if (kl_unlikely(!exprlist)) {
        klast_delete(exprlist);
        klast_delete(expr);
        return NULL;
      }
      KlAstAppend* arrpush = klast_append_create(expr, exprlist, klast_begin(expr), klast_end(exprlist));
      klparser_oomifnull(arrpush);
      return klcast(KlAstExpr*, arrpush);
    }
    default: {
      return expr;
    }
  }
}

KlAstExprList* klparser_emptyexprlist(KlParser* parser, KlLex* lex, KlFileOffset begin, KlFileOffset end) {
  KlAstExprList* exprlist = klast_exprlist_create(NULL, 0, begin, end);
  if (kl_unlikely(!exprlist)) return klparser_error_oom(parser, lex);
  return exprlist;
}

static KlAstTuple* klparser_emptytuple(KlParser* parser, KlLex* lex, KlFileOffset begin, KlFileOffset end) {
  KlAstTuple* tuple = klast_tuple_create(NULL, 0, begin, end);
  if (kl_unlikely(!tuple)) return klparser_error_oom(parser, lex);
  return tuple;
}

static KlAstExprList* klparser_singletonexprlist(KlParser* parser, KlLex* lex, KlAstExpr* expr) {
  KlAstExpr** elems = (KlAstExpr**)malloc(sizeof (KlAstExpr*));
  if (kl_unlikely(!elems)) {
    klast_delete(expr);
    return klparser_error_oom(parser, lex);
  }
  elems[0] = expr;
  KlAstExprList* exprlist = klast_exprlist_create(elems, 1, klast_begin(expr), klast_end(expr));
  if (kl_unlikely(!exprlist))
    return klparser_error_oom(parser, lex);
  return exprlist;
}

static KlAstExpr* klparser_exprunit(KlParser* parser, KlLex* lex) {
  switch (kllex_tokkind(lex)) {
    case KLTK_INT: {
      KlAstConstant* ast = klast_constant_create_integer(lex->tok.intval, kllex_tokbegin(lex), kllex_tokend(lex));
      if (kl_unlikely(!ast)) return klparser_error_oom(parser, lex);
      kllex_next(lex);
      return klcast(KlAstExpr*, ast);
    }
    case KLTK_INTDOT: {
      KlAstConstant* ast = klast_constant_create_integer(lex->tok.intval, kllex_tokbegin(lex), kllex_tokend(lex) - 1);
      if (kl_unlikely(!ast)) return klparser_error_oom(parser, lex);
      kllex_setcurrtok(lex, KLTK_DOT, kllex_tokend(lex) - 1, kllex_tokend(lex));
      return klcast(KlAstExpr*, ast);
    }
    case KLTK_FLOAT: {
      KlAstConstant* ast = klast_constant_create_float(lex->tok.floatval, kllex_tokbegin(lex), kllex_tokend(lex));
      if (kl_unlikely(!ast)) return klparser_error_oom(parser, lex);
      kllex_next(lex);
      return klcast(KlAstExpr*, ast);
    }
    case KLTK_STRING: {
      KlAstConstant* ast = klast_constant_create_string(lex->tok.string, kllex_tokbegin(lex), kllex_tokend(lex));
      if (kl_unlikely(!ast)) return klparser_error_oom(parser, lex);
      kllex_next(lex);
      return klcast(KlAstExpr*, ast);
    }
    case KLTK_BOOLVAL: {
      KlAstConstant* ast = klast_constant_create_boolean(lex->tok.boolval, kllex_tokbegin(lex), kllex_tokend(lex));
      if (kl_unlikely(!ast)) return klparser_error_oom(parser, lex);
      kllex_next(lex);
      return klcast(KlAstExpr*, ast);
    }
    case KLTK_NIL: {
      KlAstConstant* ast = klast_constant_create_nil(kllex_tokbegin(lex), kllex_tokend(lex));
      if (kl_unlikely(!ast)) return klparser_error_oom(parser, lex);
      kllex_next(lex);
      return klcast(KlAstExpr*, ast);
    }
    case KLTK_ID: {
      KlAstIdentifier* ast = klast_id_create(lex->tok.string, kllex_tokbegin(lex), kllex_tokend(lex));
      if (kl_unlikely(!ast)) return klparser_error_oom(parser, lex);
      kllex_next(lex);
      return klcast(KlAstExpr*, ast);
    }
    case KLTK_WILDCARD: {
      KlAstWildcard* ast = klast_wildcard_create(kllex_tokbegin(lex), kllex_tokend(lex));
      if (kl_unlikely(!ast)) return klparser_error_oom(parser, lex);
      kllex_next(lex);
      return klcast(KlAstExpr*, ast);
    }
    case KLTK_VARARG: {
      KlAstVararg* ast = klast_vararg_create(kllex_tokbegin(lex), kllex_tokend(lex));
      if (kl_unlikely(!ast)) return klparser_error_oom(parser, lex);
      kllex_next(lex);
      return klcast(KlAstExpr*, ast);
    }
    case KLTK_LBRACKET: {
      return klcast(KlAstExpr*, klparser_array(parser, lex));
    }
    case KLTK_LBRACE: {
      return klparser_exprbrace(parser, lex);
    }
    case KLTK_LPAREN: {
      KlFileOffset begin = kllex_tokbegin(lex);
      kllex_next(lex);
      KlFileOffset mayend = kllex_tokend(lex);
      if (kllex_trymatch(lex, KLTK_RPAREN)) { /* maybe an exprlist */
        return kllex_check(lex, KLTK_ARROW) || kllex_check(lex, KLTK_DARROW)
               ? klcast(KlAstExpr*, klparser_emptyexprlist(parser, lex, begin, mayend))
               : klcast(KlAstExpr*, klparser_emptytuple(parser, lex, begin, mayend));
      }

      KlAstExpr* expr = klparser_expr(parser, lex);
      if (kllex_check(lex, KLTK_RPAREN)) {  /* maybe an exprlist */
        KlFileOffset end = kllex_tokend(lex);
        kllex_next(lex);
        klparser_returnifnull(expr);
        klast_setposition(expr, begin, end);
        return kllex_check(lex, KLTK_ARROW) || kllex_check(lex, KLTK_DARROW)
               ? klcast(KlAstExpr*, klparser_singletonexprlist(parser, lex, expr))
               : expr;
      }
      /* is an exprlist or a tuple */
      klparser_returnifnull(expr);
      size_t nexpr;
      KlAstExpr** exprs = klparser_finishcommasepexpr(parser, lex, expr, &nexpr);
      klparser_returnifnull(exprs);
      KlFileOffset end = kllex_tokend(lex);
      klparser_match(parser, lex, KLTK_RPAREN);
      if (kllex_check(lex, KLTK_ARROW) || kllex_check(lex, KLTK_DARROW)) {  /* is an exprlist */
        KlAstExprList* exprlist = klast_exprlist_create(exprs, nexpr, begin, end);
        klparser_oomifnull(exprlist);
        return klcast(KlAstExpr*, exprlist);
      } else {  /* is a tuple */
        KlAstTuple* tuple = klast_tuple_create(exprs, nexpr, begin, end);
        klparser_oomifnull(tuple);
        return klcast(KlAstExpr*, tuple);
      }
    }
    case KLTK_NEW: {
      return klcast(KlAstExpr*, klparser_exprnew(parser, lex));
    }
    default: {
      klparser_error(parser, kllex_inputstream(lex), kllex_tokbegin(lex), kllex_tokend(lex),
                     "expected '(', '{', '[', true, false, identifier, string, new or integer");
      return NULL;
    }
  }
}

static KlAstExpr* klparser_array(KlParser* parser, KlLex* lex) {
  kl_assert(kllex_check(lex, KLTK_LBRACKET), "expected \'[\'");
  KlFileOffset begin = kllex_tokbegin(lex);
  kllex_next(lex);  /* skip '[' */
  KlAstExprList* exprlist = klparser_exprlist_mayempty(parser, lex);
  if (kl_unlikely(!exprlist)) {
    klparser_discardto(lex, KLTK_RBRACKET);
    return NULL;
  }
  if (kllex_trymatch(lex, KLTK_BAR)) { /* array comprehension */
    KlAstIdentifier* arrid = klast_id_create(klparser_newtmpid(parser, lex), klast_begin(exprlist), klast_end(exprlist));
    klparser_oomifnull(arrid);
    KlAstAppend* exprpush = klast_append_create(klcast(KlAstExpr*, arrid), exprlist, klast_begin(exprlist), klast_end(exprlist));
    klparser_oomifnull(exprpush);
    KlAstExprList* single_expr = klparser_singletonexprlist(parser, lex, klcast(KlAstExpr*, exprpush));
    klparser_oomifnull(single_expr);
    KlAstStmtExpr* inner_stmt = klast_stmtexpr_create(single_expr, klast_begin(exprlist), klast_end(exprlist));
    klparser_oomifnull(inner_stmt);
    KlAstStmtList* stmtlist = klparser_comprehension(parser, lex, klcast(KlAstStmt*, inner_stmt));
    KlFileOffset end = kllex_tokend(lex);
    klparser_match(parser, lex, KLTK_RBRACKET);
    klparser_returnifnull(stmtlist);  /* 'inner_stmt' will be deleted in klparser_comprehension() when error occurred */
    KlAstArrayComprehension* array = klast_arraycomprehension_create(arrid->id, stmtlist, begin, end);
    return klcast(KlAstExpr*, array);
  } else {
    KlFileOffset end = kllex_tokend(lex);
    klparser_match(parser, lex, KLTK_RBRACKET);
    KlAstArray* array = klast_array_create(exprlist, begin, end);
    klparser_oomifnull(array);
    return klcast(KlAstExpr*, array);
  }
}

static KlAstExpr** klparser_finishcommasepexpr(KlParser* parser, KlLex* lex, KlAstExpr* expr, size_t* pnelem) {
  KArray exprarray;
  if (kl_unlikely(!karray_init(&exprarray))) {
    klast_delete(expr);
    return klparser_error_oom(parser, lex);
  }
  if (kl_unlikely(!karray_push_back(&exprarray, expr))) {
    klparser_destroy_astarray(&exprarray);
    klast_delete(expr);
    return klparser_error_oom(parser, lex);
  }
  while (kllex_trymatch(lex, KLTK_COMMA) && klparser_exprbegin(lex)) {
    KlAstExpr* expr = klparser_expr(parser, lex);
    if (kl_unlikely(!expr)) continue;
    klparser_karr_pushast(&exprarray, expr);
  }
  kl_assert(karray_size(&exprarray) != 0, "");
  karray_shrink(&exprarray);
  size_t nelem = karray_size(&exprarray);
  KlAstExpr** exprs = (KlAstExpr**)karray_steal(&exprarray);
  *pnelem = nelem;
  return exprs;
}

KlAstExprList* klparser_finishexprlist(KlParser* parser, KlLex* lex, KlAstExpr* expr) {
  size_t nelem;
  KlAstExpr** exprs = klparser_finishcommasepexpr(parser, lex, expr, &nelem);
  klparser_returnifnull(exprs);
  KlAstExprList* exprlist = klast_exprlist_create(exprs, nelem, klast_begin(expr), klast_end(exprs[nelem - 1]));
  klparser_oomifnull(exprlist);
  return exprlist;
}

static KlAstExpr* klparser_exprbrace(KlParser* parser, KlLex* lex) {
  kl_assert(kllex_check(lex, KLTK_LBRACE), "");
  KlFileOffset begin = kllex_tokbegin(lex);
  kllex_next(lex);
  KlAstExpr* expr = klparser_exprbrace_inner(parser, lex);
  KlFileOffset end = kllex_tokend(lex);
  klparser_match(parser, lex, KLTK_RBRACE);
  klparser_returnifnull(expr);
  klast_setposition(expr, begin, end);
  return expr;
}

static KlAstExpr* klparser_exprbrace_inner(KlParser* parser, KlLex* lex) {
  if (kllex_check(lex, KLTK_RBRACE) || kllex_check(lex, KLTK_LOCAL) ||
      kllex_check(lex, KLTK_SHARED) || kllex_check(lex, KLTK_METHOD)) {  /* is class */
    return klcast(KlAstExpr*, klparser_finishclass(parser, lex, (KlStrDesc) { .id = 0, .length = 0 }, NULL, false));
  }

  if (kllex_trymatch(lex, KLTK_COLON)) {  /* empty map */
    return klparser_finishmap(parser, lex, NULL, NULL);
  }

  KlAstExprList* exprlist = klparser_exprlist(parser, lex);
  if (kl_unlikely(!exprlist)) {
    klparser_discarduntil(lex, KLTK_RBRACE);
    return NULL;
  }
  if (kllex_trymatch(lex, KLTK_ASSIGN)) { /* a class? */
    if (kl_unlikely(exprlist->nexpr != 1                            ||
                    klast_kind(exprlist->exprs[0]) != KLAST_EXPR_ID)) {
      klparser_error(parser, kllex_inputstream(lex),
                     klast_begin(exprlist), klast_end(exprlist),
                     "should be a single identifier in class definition");
      klast_delete(exprlist);
      klparser_discarduntil(lex, KLTK_RBRACE);
      return NULL;
    }
    KlStrDesc id = klcast(KlAstIdentifier*, exprlist->exprs[0])->id;
    klast_delete(exprlist);
    KlAstExpr* expr = klparser_expr(parser, lex);
    return klcast(KlAstExpr*, klparser_finishclass(parser, lex, id, expr, true));
  } else if (kllex_trymatch(lex, KLTK_COLON)) { /* a map? */
    if (kl_unlikely(exprlist->nexpr != 1)) {
      klparser_error(parser, kllex_inputstream(lex),
                     klast_begin(exprlist), klast_end(exprlist),
                     "should be a single expression in map constructor");
    }
    KlAstExpr* key = klast_exprlist_stealfirst_and_destroy(exprlist);
    KlAstExpr* val = klparser_expr(parser, lex);
    if (kl_unlikely(!val)) {
      klast_delete(key);
      /* pretend there are no pre-parsed k-v pair to recover from syntax error */
      key = NULL;
    }
    return klparser_finishmap(parser, lex, key, val);
  } else {
    /* else is a comprehension */
    klparser_match(parser, lex, KLTK_BAR);
    KlAstYield* yieldexpr = klast_yield_create(exprlist, klast_begin(exprlist), klast_end(exprlist));
    klparser_oomifnull(yieldexpr);
    KlAstExprList* single_expr = klparser_singletonexprlist(parser, lex, klcast(KlAstExpr*, yieldexpr));
    klparser_oomifnull(single_expr);
    KlAstStmtExpr* stmt = klast_stmtexpr_create(single_expr, klast_begin(yieldexpr),  klast_end(yieldexpr));
    klparser_oomifnull(stmt);
    KlAstStmtList* comprehension = klparser_comprehension(parser, lex, klcast(KlAstStmt*, stmt));
    klparser_returnifnull(comprehension);
    KlAstExprList* params = klparser_emptyexprlist(parser, lex, klast_begin(stmt), klast_begin(stmt));
    if (kl_unlikely(!params)) {
      klast_delete(comprehension);
      return NULL;
    }
    KlAstFunc* func = klast_func_create(comprehension, params, false, klast_begin(stmt), klast_end(comprehension));
    klparser_oomifnull(func);
    KlAstAsync* asyncexpr = klast_async_create(klcast(KlAstExpr*, func), KLPARSER_ERROR_PH_FILEPOS, KLPARSER_ERROR_PH_FILEPOS);
    klparser_oomifnull(asyncexpr);
    return klcast(KlAstExpr*, asyncexpr);
  }
}

static KlAstExpr* klparser_finishmap(KlParser* parser, KlLex* lex, KlAstExpr* firstkey, KlAstExpr* firstval) {
  /* if firstkey is NULL, then the map is not preparsed */
  KArray keys;
  KArray vals;
  if (kl_unlikely(!karray_init(&keys) || !karray_init(&vals))) {
    karray_destroy(&keys);
    karray_destroy(&vals);
    if (firstkey) klast_delete(firstkey);
    if (firstval) klast_delete(firstval);
    return klparser_error_oom(parser, lex);
  }

  if (firstkey) {
    klparser_karr_pushast(&keys, firstkey);
    klparser_karr_pushast(&vals, firstval);
    kllex_trymatch(lex, KLTK_COMMA);
  }

  while (klparser_exprbegin(lex)) {
    KlAstExpr* key = klparser_expr(parser, lex);
    klparser_match(parser, lex, KLTK_COLON);
    KlAstExpr* val = klparser_expr(parser, lex);
    if (kl_likely(key && val)) {
      klparser_karr_pushast(&keys, key);
      klparser_karr_pushast(&vals, val);
    } else {
      if (key) klast_delete(key);
      if (val) klast_delete(val);
    }
    kllex_trymatch(lex, KLTK_COMMA);
  }

  if (kl_unlikely(karray_size(&keys) != karray_size(&vals))) {
    klparser_destroy_astarray(&keys);
    klparser_destroy_astarray(&vals);
    return NULL;
  }

  karray_shrink(&keys);
  karray_shrink(&vals);
  size_t npair = karray_size(&keys);
  KlAstExpr** keys_stolen = (KlAstExpr**)karray_steal(&keys);
  KlAstExpr** vals_stolen = (KlAstExpr**)karray_steal(&vals);

  if (kllex_check(lex, KLTK_BAR)) { /* is map comprehension */
    return klcast(KlAstExpr*, klparser_finishmapcomprehension(parser, lex, keys_stolen, vals_stolen, npair));
  } else {  /* a normal map */
    KlAstMap* map = klast_map_create(keys_stolen, vals_stolen, npair, KLPARSER_ERROR_PH_FILEPOS, KLPARSER_ERROR_PH_FILEPOS);
    klparser_oomifnull(map);
    return klcast(KlAstExpr*, map);
  }
}

static void klparser_mapcomprehension_helper_clean(KlAstExpr** keys, KlAstExpr** vals, size_t npair) {
  for (size_t i = 0; i < npair; ++i) {
    if (keys[i]) klast_delete(keys[i]);
    if (vals[i]) klast_delete(vals[i]);
  }
  free(keys);
  free(vals);
}

static KlAstMapComprehension* klparser_finishmapcomprehension(KlParser* parser, KlLex* lex, KlAstExpr** keys, KlAstExpr** vals, size_t npair) {
  kl_assert(kllex_check(lex, KLTK_BAR), "");
  KlFileOffset inner_begin = npair == 0 ? kllex_tokbegin(lex) : klast_begin(keys[0]);
  KlFileOffset inner_end = npair == 0 ? kllex_tokend(lex) : klast_end(vals[npair - 1]);
  kllex_next(lex);

  KlStrDesc tmpmapid = klparser_newtmpid(parser, lex);
  for (size_t i = 0; i < npair; ++i) {
    KlAstIdentifier* mapid = klast_id_create(tmpmapid, inner_begin, inner_begin);
    if (kl_unlikely(!mapid)) {
      klparser_mapcomprehension_helper_clean(keys, vals, npair);
      return klparser_error_oom(parser, lex);
    }
    keys[i] = klcast(KlAstExpr*, klast_index_create(klcast(KlAstExpr*, mapid), keys[i], klast_begin(keys[i]), klast_end(keys[i])));
    if (kl_unlikely(!keys[i])) {
      klparser_mapcomprehension_helper_clean(keys, vals, npair);
      return klparser_error_oom(parser, lex);
    }
  }

  KlAstExprList* lvals = klast_exprlist_create(keys, npair, inner_begin, inner_end);
  KlAstExprList* rvals = klast_exprlist_create(vals, npair, inner_begin, inner_end);
  if (kl_unlikely(!lvals || !rvals)) {
    if (lvals) klast_delete(lvals);
    if (rvals) klast_delete(rvals);
    return klparser_error_oom(parser, lex);
  }
  KlAstStmtAssign* inner_stmt = klast_stmtassign_create(lvals, rvals, inner_begin, inner_end);
  klparser_oomifnull(inner_stmt);

  KlAstStmtList* stmts = klparser_comprehension(parser, lex, klcast(KlAstStmt*, inner_stmt));
  klparser_returnifnull(stmts);
  KlAstMapComprehension* mapgen = klast_mapcomprehension_create(tmpmapid, stmts, KLPARSER_ERROR_PH_FILEPOS, KLPARSER_ERROR_PH_FILEPOS);
  klparser_oomifnull(mapgen);
  return mapgen;
}

static KlAstClass* klparser_finishclass(KlParser* parser, KlLex* lex, KlStrDesc id, KlAstExpr* expr, bool preparsed) {
  KlCfdArray fields;
  KArray vals;
  if (kl_unlikely(!klcfd_init(&fields, 16) || !karray_init(&vals))) {
    klcfd_destroy(&fields);
    karray_destroy(&vals);
    return klparser_error_oom(parser, lex);
  }

  if (preparsed) {
    /* insert first shared k-v pair */
    KlAstClassFieldDesc cfd;
    cfd.shared = true;
    cfd.name = id;
    if (kl_unlikely(!klcfd_push_back(&fields, &cfd)))
      klparser_error_oom(parser, lex);
    /* NULL expr is OK for class, this field will be traited as a field with no initial value. */
    if (kl_unlikely(!karray_push_back(&vals, expr))) {
      if (expr) klast_delete(expr);
      klparser_error_oom(parser, lex);
    }
    if (kllex_trymatch(lex, KLTK_COMMA))  /* has trailing shared k-v pairs */
      klparser_sharedlist(parser, lex, &fields, &vals);
    kllex_trymatch(lex, KLTK_SEMI);
  }

  while (!kllex_check(lex, KLTK_RBRACE)) {
    if (kllex_trymatch(lex, KLTK_LOCAL)) {
      klparser_locallist(parser, lex, &fields, &vals);
      kllex_trymatch(lex, KLTK_SEMI);
    } else if (kllex_trymatch(lex, KLTK_METHOD)) {
      klparser_classmethod(parser, lex, &fields, &vals);
      kllex_trymatch(lex, KLTK_SEMI);
    } else if (kl_likely(!kllex_check(lex, KLTK_END))) {
      kllex_trymatch(lex, KLTK_SHARED);
      klparser_sharedlist(parser, lex, &fields, &vals);
      kllex_trymatch(lex, KLTK_SEMI);
    } else {
      klparser_error(parser, kllex_inputstream(lex), kllex_tokbegin(lex), kllex_tokend(lex), "end of file");
      break;
    }
  }

  if (kl_unlikely(klcfd_size(&fields) != karray_size(&vals))) {
    klcfd_destroy(&fields);
    klparser_destroy_astarray(&vals);
    return NULL;
  }

  klcfd_shrink(&fields);
  karray_shrink(&vals);
  size_t nfield = karray_size(&vals);
  KlAstClass* klclass = klast_class_create(klcfd_steal(&fields), (KlAstExpr**)karray_steal(&vals), nfield, NULL, KLPARSER_ERROR_PH_FILEPOS, KLPARSER_ERROR_PH_FILEPOS);
  klparser_oomifnull(klclass);
  return klclass;
}

static void klparser_locallist(KlParser* parser, KlLex* lex, KlCfdArray* fields, KArray* vals) {
  KlAstClassFieldDesc fielddesc;
  fielddesc.shared = false;
  fielddesc.ismethod = false;
  do {
    if (kl_unlikely(!klparser_check(parser, lex, KLTK_ID)))
      return;
    fielddesc.name = lex->tok.string;
    if (kl_unlikely(!klcfd_push_back(fields, &fielddesc) || !karray_push_back(vals, NULL))) {
      klparser_error_oom(parser, lex);
    }
    kllex_next(lex);
  } while (kllex_trymatch(lex, KLTK_COMMA));
}

static void klparser_sharedlist(KlParser* parser, KlLex* lex, KlCfdArray* fields, KArray* vals) {
  KlAstClassFieldDesc fielddesc;
  fielddesc.shared = true;
  fielddesc.ismethod = false;
  do {
    if (kl_unlikely(!klparser_check(parser, lex, KLTK_ID)))
      return;
    fielddesc.name = lex->tok.string;
    kllex_next(lex);
    klparser_match(parser, lex, KLTK_ASSIGN);
    KlAstExpr* val = klparser_expr(parser, lex);
    if (kl_unlikely(!klcfd_push_back(fields, &fielddesc)))
      klparser_error_oom(parser, lex);
    klparser_karr_pushast(vals, val);
  } while (kllex_trymatch(lex, KLTK_COMMA));
}

static void klparser_classmethod(KlParser* parser, KlLex* lex, KlCfdArray* fields, KArray* vals) {
  KlAstClassFieldDesc fielddesc;
  fielddesc.shared = true;
  fielddesc.ismethod = true;
  if (kl_unlikely(!klparser_check(parser, lex, KLTK_ID)))
    return;
  fielddesc.name = lex->tok.string;
  kllex_next(lex);
  klparser_match(parser, lex, KLTK_ASSIGN);
  KlAstExpr* val = klparser_expr(parser, lex);
  if (kl_unlikely(!klcfd_push_back(fields, &fielddesc)))
    klparser_error_oom(parser, lex);
  klparser_karr_pushast(vals, val);
}

static KlAstExpr* klparser_dotchain(KlParser* parser, KlLex* lex) {
  KlAstExpr* dotexpr = klparser_exprunit(parser, lex);
  while (kllex_trymatch(lex, KLTK_DOT)) {
    if (kl_unlikely(!klparser_check(parser, lex, KLTK_ID)))
      continue;
    if (kl_unlikely(!dotexpr)) {
      kllex_next(lex);
      continue;
    }
    KlAstDot* dot = klast_dot_create(dotexpr, lex->tok.string, klast_begin(dotexpr), kllex_tokend(lex));
    klparser_oomifnull(dot);
    dotexpr = klcast(KlAstExpr*, dot);
    kllex_next(lex);
  }
  return dotexpr;
}

static KlAstNew* klparser_exprnew(KlParser* parser, KlLex* lex) {
  kl_assert(kllex_check(lex, KLTK_NEW), "");

  KlFileOffset begin = kllex_tokbegin(lex);
  kllex_next(lex);
  KlAstExpr* klclass = kllex_check(lex, KLTK_LPAREN)
                     ? klparser_exprunit(parser, lex)
                     : klparser_dotchain(parser, lex);
  if (kllex_trymatch(lex, KLTK_LPAREN)) { /* has initialization list */
    KlAstExprList* args = klparser_exprlist_mayempty(parser, lex);
    KlFileOffset end = kllex_tokend(lex);
    klparser_match(parser, lex, KLTK_RPAREN);
    if (kl_unlikely(!klclass || !args)) {
      if (klclass) klast_delete(klclass);
      if (args) klast_delete(args);
      return NULL;
    }
    KlAstNew* newexpr = klast_new_create(klclass, klcast(KlAstExprList*, args), begin, end);
    klparser_oomifnull(newexpr);
    return newexpr;
  } else {
    klparser_returnifnull(klclass);
    KlAstNew* newexpr = klast_new_create(klclass, NULL, begin, klast_end(klclass));
    klparser_oomifnull(newexpr);
    return newexpr;
  }
}

static KlAstExpr* klparser_exprpre(KlParser* parser, KlLex* lex) {
  switch (kllex_tokkind(lex)) {
    case KLTK_MINUS: {
      KlFileOffset begin = kllex_tokbegin(lex);
      kllex_next(lex);
      KlAstExpr* expr = klparser_exprpre(parser, lex);
      klparser_returnifnull(expr);
      KlAstPre* neg = klast_pre_create(KLTK_MINUS, expr, begin, klast_end(expr));
      klparser_oomifnull(neg);
      return klcast(KlAstExpr*, neg);
    }
    case KLTK_LEN: {
      KlFileOffset begin = kllex_tokbegin(lex);
      kllex_next(lex);
      KlAstExpr* expr = klparser_exprpre(parser, lex);
      klparser_returnifnull(expr);
      KlAstPre* notexpr = klast_pre_create(KLTK_LEN, expr, begin, klast_end(expr));
      klparser_oomifnull(notexpr);
      return klcast(KlAstExpr*, notexpr);
    }
    case KLTK_NOT: {
      KlFileOffset begin = kllex_tokbegin(lex);
      kllex_next(lex);
      KlAstExpr* expr = klparser_exprpre(parser, lex);
      klparser_returnifnull(expr);
      KlAstPre* notexpr = klast_pre_create(KLTK_NOT, expr, begin, klast_end(expr));
      klparser_oomifnull(notexpr);
      return klcast(KlAstExpr*, notexpr);
    }
    case KLTK_INHERIT: {
      KlFileOffset begin = kllex_tokbegin(lex);
      kllex_next(lex);
      KlAstExpr* base = klparser_exprpre(parser, lex);
      klparser_returnifnull(base);
      klparser_match(parser, lex, KLTK_COLON);
      KlAstExpr* ast = klparser_exprunit(parser, lex);
      if (kl_unlikely(!ast)) {
        klast_delete(base);
        return NULL;
      }
      if (kl_unlikely(klast_kind(ast) != KLAST_EXPR_CLASS)) {
        klparser_error(parser, kllex_inputstream(lex), klast_begin(ast), klast_end(ast),
                       "must be a class definition");
        klast_delete(base);
        klast_delete(ast);
        return NULL;
      }
      KlAstClass* klclass = klcast(KlAstClass*, ast);
      klclass->baseclass = base;
      klast_setposition(klclass, begin, klast_end(klclass));
      return klcast(KlAstExpr*, klclass);
    }
    case KLTK_ADD: {
      kllex_next(lex);
      return klparser_exprpre(parser, lex);
    }
    default: {  /* no prefix */
      return klparser_exprpost(parser, lex);
    }
  }
}

static KlAstExprList* klparser_correctfuncparams(KlParser* parser, KlLex* lex, KlAstExpr* expr, bool* vararg) {
  *vararg = false;
  if (klast_kind(expr) == KLAST_EXPR_LIST) {
    KlAstExprList* params = klcast(KlAstExprList*, expr);
    KlAstExpr** exprs = params->exprs;
    size_t nexpr = params->nexpr;
    for (size_t i = 0; i < nexpr; ++i) {
      if (klast_kind(exprs[i]) == KLAST_EXPR_VARARG) {
        if (kl_unlikely(i != nexpr - 1))
          klparser_error(parser, kllex_inputstream(lex), klast_begin(exprs[i]), klast_end(exprs[i]), "'...' can only appear at the end of the parameter list");
        *vararg = true;
        continue;
      }
    }
    if (*vararg) {
      klast_delete(params->exprs[params->nexpr - 1]);
      --params->nexpr;
    }
    return params;
  } else if (klast_kind(expr) == KLAST_EXPR_VARARG) {
    *vararg = true;
    KlFileOffset begin = klast_begin(expr);
    KlFileOffset end = klast_end(expr);
    klast_delete(expr);
    return klparser_emptyexprlist(parser, lex, begin, end);
  } else {
    return klparser_singletonexprlist(parser, lex, expr);
  }
}

static KlAstStmtList* klparser_darrowfuncbody(KlParser* parser, KlLex* lex) {
  kl_assert(kllex_check(lex, KLTK_DARROW), "");
  kllex_next(lex);
  return klparser_stmtblock(parser, lex);
}

static KlAstStmtList* klparser_arrowfuncbody(KlParser* parser, KlLex* lex) {
  kl_assert(kllex_check(lex, KLTK_ARROW), "");
  kllex_next(lex);
  KlAstExpr* expr = klparser_expr(parser, lex);
  klparser_returnifnull(expr);
  KlAstExprList* retvals = klparser_singletonexprlist(parser, lex, expr);
  klparser_returnifnull(retvals);
  KlAstStmtReturn* stmtreturn = klast_stmtreturn_create(retvals, klast_begin(expr), klast_end(expr));
  klparser_oomifnull(stmtreturn);
  KlAstStmt** stmts = (KlAstStmt**)malloc(sizeof (KlAstStmt**));
  if (kl_unlikely(!stmts)) {
    klast_delete(stmtreturn);
    return klparser_error_oom(parser, lex);
  }
  stmts[0] = klcast(KlAstStmt*, stmtreturn);
  KlAstStmtList* block = klast_stmtlist_create(stmts, 1, klast_begin(stmtreturn), klast_end(stmtreturn));
  klparser_oomifnull(block);
  return block;
}

static KlAstCall* klparser_exprcall(KlParser* parser, KlLex* lex, KlAstExpr* callable) {
  bool is_singleton = kllex_hasleadingblank(lex);
  if (is_singleton) {
    KlAstExpr* unit = klparser_exprunit(parser, lex);
    if (kl_unlikely(!unit)) {
      if (callable) klast_delete(callable);
      return NULL;
    }
    KlAstExprList* params = klparser_singletonexprlist(parser, lex, unit);
    if (kl_unlikely(!params)) {
      if (callable) klast_delete(callable);
      return NULL;
    }
    if (kl_unlikely(!callable)) {
      klast_delete(params);
      return NULL;
    }
    KlAstCall* call = klast_call_create(callable, params, klast_begin(callable), klast_end(unit));
    klparser_oomifnull(call);
    return call;
  } else {
    KlFileOffset begin = kllex_tokbegin(lex);
    klparser_match(parser, lex, KLTK_LPAREN);
    KlAstExprList* params = klparser_exprlist_mayempty(parser, lex);
    KlFileOffset end = kllex_tokend(lex);
    klparser_match(parser, lex, KLTK_RPAREN);
    if (kl_unlikely(!params)) {
      if (callable) klast_delete(callable);
      return NULL;
    }
    if (kl_unlikely(!callable)) {
      klast_delete(params);
      return NULL;
    }
    klast_setposition(params, begin, end);
    KlAstCall* call = klast_call_create(callable, params, klast_begin(callable), klast_end(params));
    klparser_oomifnull(call);
    return call;
  }
}

static KlAstWhere* klparser_exprfinishwhere(KlParser* parser, KlLex* lex, KlAstExpr* expr) {
  kllex_next(lex);
  KlAstStmtList* block = klparser_stmtblock(parser, lex);
  if (kl_unlikely(!block)) {
    klast_delete(expr);
    return NULL;
  }
  KlAstWhere* exprwhere = klast_where_create(expr, block, klast_begin(expr), klast_end(block));
  klparser_oomifnull(exprwhere);
  return exprwhere;
}

static KlAstMatch* klparser_exprmatch(KlParser* parser, KlLex* lex) {
  kl_assert(kllex_check(lex, KLTK_CASE), "expected 'case'");
  KlFileOffset begin = kllex_tokbegin(lex);
  kllex_next(lex);
  KlAstExpr* matchobj = klparser_expr(parser, lex);
  klparser_match(parser, lex, KLTK_OF);
  KArray patterns;
  KArray exprs;
  if (kl_unlikely(!karray_init(&patterns) || !karray_init(&exprs))) {
    karray_destroy(&patterns);
    karray_destroy(&exprs);
    if (matchobj) klast_delete(matchobj);
    return klparser_error_oom(parser, lex);
  }
  do {
    KlAstExpr* pattern = klparser_expr(parser, lex);
    klparser_match(parser, lex, KLTK_ASSIGN);
    KlAstExpr* expr = klparser_expr(parser, lex);
    if (kl_unlikely(!pattern || !expr)) {
      if (pattern) klast_delete(pattern);
      if (expr) klast_delete(expr);
    } else {
      klparser_karr_pushast(&patterns, pattern);
      klparser_karr_pushast(&exprs, expr);
    }
  } while (kllex_trymatch(lex, KLTK_COMMA));
  if (kl_unlikely(karray_size(&patterns) != karray_size(&exprs) ||
                  karray_size(&patterns) == 0 ||
                  !matchobj)) {
    klparser_destroy_astarray(&patterns);
    klparser_destroy_astarray(&exprs);
    if (matchobj) klast_delete(matchobj);
    return NULL;
  }
  karray_shrink(&patterns);
  karray_shrink(&exprs);
  size_t npattern = karray_size(&patterns);
  KlAstExpr** patterns_stolen = klcast(KlAstExpr**, karray_steal(&patterns));
  KlAstExpr** exprs_stolen = klcast(KlAstExpr**, karray_steal(&exprs));
  KlAstMatch* matchast = klast_match_create(matchobj, patterns_stolen, exprs_stolen, npattern, begin, klast_end(exprs_stolen[npattern - 1]));
  klparser_oomifnull(matchast);
  return matchast;
}

static KlAstExpr* klparser_exprpost(KlParser* parser, KlLex* lex) {
  KlAstExpr* postexpr = klparser_exprunit(parser, lex);
  while (true) {
    switch (kllex_tokkind(lex)) {
      case KLTK_LBRACKET: { /* index */
        if (kllex_hasleadingblank(lex)) {
          postexpr = klcast(KlAstExpr*, klparser_exprcall(parser, lex, postexpr));
          break;
        }
        kllex_next(lex);
        KlAstExpr* expr = klparser_expr(parser, lex);
        KlFileOffset end = kllex_tokend(lex);
        klparser_match(parser, lex, KLTK_RBRACKET);
        if (kl_unlikely(!expr)) break;
        if (kl_unlikely(!postexpr)) {
          klast_delete(expr);
          break;
        }
        KlAstIndex* index = klast_index_create(postexpr, expr, klast_begin(postexpr), end);
        klparser_oomifnull(index);
        postexpr = klcast(KlAstExpr*, index);
        break;
      }
      case KLTK_DOT: {
        kllex_next(lex);
        if (kl_unlikely(!klparser_check(parser, lex, KLTK_ID) || !postexpr))
          break;
        KlAstDot* dot = klast_dot_create(postexpr, lex->tok.string, klast_begin(postexpr), kllex_tokend(lex));
        klparser_oomifnull(dot);
        postexpr = klcast(KlAstExpr*, dot);
        kllex_next(lex);
        break;
      }
      case KLTK_ID:
      case KLTK_STRING:
      case KLTK_INT:
      case KLTK_INTDOT:
      case KLTK_FLOAT:
      case KLTK_BOOLVAL:
      case KLTK_NIL:
      case KLTK_VARARG:
      case KLTK_NEW:
      case KLTK_LPAREN:
      case KLTK_LBRACE: { /* function call */
        postexpr = klcast(KlAstExpr*, klparser_exprcall(parser, lex, postexpr));
        break;
      }
      default: {  /* no more postfix, just return */
        return postexpr;
      }
    }
  }
}

static KlAstExpr* klparser_exprbin(KlParser* parser, KlLex* lex, int prio) {
  static const int binop_prio[KLTK_NTOKEN] = {
    [KLTK_OR]     = 1,
    [KLTK_AND]    = 2,
    [KLTK_LE]     = 3,
    [KLTK_LT]     = 3,
    [KLTK_GE]     = 3,
    [KLTK_GT]     = 3,
    [KLTK_EQ]     = 3,
    [KLTK_NE]     = 3,
    [KLTK_IS]     = 3,
    [KLTK_ISNOT]  = 3,
    [KLTK_CONCAT] = 4,
    [KLTK_ADD]    = 5,
    [KLTK_MINUS]  = 5,
    [KLTK_MUL]    = 6,
    [KLTK_DIV]    = 6,
    [KLTK_MOD]    = 6,
    [KLTK_IDIV]   = 6,
  };
  KlAstExpr* left = klparser_exprpre(parser, lex);
  KlTokenKind op = kllex_tokkind(lex);
  while (kltoken_isbinop(op) && binop_prio[op] > prio) {
    kllex_next(lex);
    KlAstExpr* right = klparser_exprbin(parser, lex, binop_prio[op]);
    if (kl_unlikely(!left || !right)) {
      if (left) klast_delete(left);
      if (right) klast_delete(right);
      return NULL;
    }
    KlAstBin* binexpr = klast_bin_create(op, left, right, klast_begin(left), klast_end(right));
    klparser_oomifnull(binexpr);
    left = klcast(KlAstExpr*, binexpr);
    op = kllex_tokkind(lex);
  }
  return klcast(KlAstExpr*, left);
}

const bool klparser_isexprbegin[KLTK_NTOKEN] = {
  [KLTK_ARROW] = true,
  [KLTK_DARROW] = true,
  [KLTK_VARARG] = true,
  [KLTK_LEN] = true,
  [KLTK_MINUS] = true,
  [KLTK_ADD] = true,
  [KLTK_NOT] = true,
  [KLTK_NEW] = true,
  [KLTK_INT] = true,
  [KLTK_INTDOT] = true,
  [KLTK_FLOAT] = true,
  [KLTK_STRING] = true,
  [KLTK_BOOLVAL] = true,
  [KLTK_NIL] = true,
  [KLTK_WILDCARD] = true,
  [KLTK_ID] = true,
  [KLTK_LBRACKET] = true,
  [KLTK_LBRACE] = true,
  [KLTK_YIELD] = true,
  [KLTK_ASYNC] = true,
  [KLTK_CASE] = true,
  [KLTK_INHERIT] = true,
  [KLTK_LPAREN] = true,
};

