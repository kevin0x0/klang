#include "include/parse/klparser_expr.h"
#include "include/parse/klparser_stmt.h"
#include "include/parse/klparser_generator.h"
#include "include/array/karray.h"
#include "include/parse/klcfdarr.h"
#include "include/parse/klparser_error.h"
#include "include/parse/klparser_utils.h"


static KlAstStmtList* klparser_darrowfuncbody(KlParser* parser, KlLex* lex);
static KlAstStmtList* klparser_arrowfuncbody(KlParser* parser, KlLex* lex);
/* parse all syntactical structure that begin with '{"(class, map, map generator, coroutine generator) */
static KlAst* klparser_exprbrace(KlParser* parser, KlLex* lex);
static KlAst* klparser_exprbrace_inner(KlParser* parser, KlLex* lex);
static KlAst* klparser_finishmap(KlParser* parser, KlLex* lex, KlAst* firstkey, KlAst* firstval);
static KlAstMapGenerator* klparser_finishmapgenerator(KlParser* parser, KlLex* lex, KlAst** keys, KlAst** vals, size_t npair);
static KlAstClass* klparser_finishclass(KlParser* parser, KlLex* lex, KlStrDesc id, KlAst* expr, bool preparsed);
static void klparser_locallist(KlParser* parser, KlLex* lex, KlCfdArray* fields, KArray* vals);
static void klparser_sharedlist(KlParser* parser, KlLex* lex, KlCfdArray* fields, KArray* vals);
static KlAst* klparser_array(KlParser* parser, KlLex* lex);
static KlAst* klparser_dotchain(KlParser* parser, KlLex* lex);
static KlAstNew* klparser_exprnew(KlParser* parser, KlLex* lex);
static KlAstWhere* klparser_exprfinishwhere(KlParser* parser, KlLex* lex, KlAst* expr);
static KlAstMatch* klparser_exprmatch(KlParser* parser, KlLex* lex);
static KlAst* klparser_exprunit(KlParser* parser, KlLex* lex, bool* inparenthesis);
static KlAst* klparser_exprpost(KlParser* parser, KlLex* lex);
static KlAst* klparser_exprpre(KlParser* parser, KlLex* lex);
static KlAst* klparser_exprbin(KlParser* parser, KlLex* lex, int prio);


KlAst* klparser_expr(KlParser* parser, KlLex* lex) {
  if (kllex_check(lex, KLTK_CASE))
    return klast(klparser_exprmatch(parser, lex));
  KlAst* expr = klparser_exprbin(parser, lex, 0);
  if (kl_unlikely(!expr)) return NULL;
  return kllex_check(lex, KLTK_WHERE) ? klast(klparser_exprfinishwhere(parser, lex, expr)) :
                                        expr;
}

KlAstExprList* klparser_emptyexprlist(KlParser* parser, KlLex* lex, KlFileOffset begin, KlFileOffset end) {
  KlAstExprList* exprlist = klast_exprlist_create(NULL, 0, begin, end);
  if (kl_unlikely(!exprlist)) return klparser_error_oom(parser, lex);
  return exprlist;
}

static KlAstExprList* klparser_singletonexprlist(KlParser* parser, KlLex* lex, KlAst* expr) {
  KlAst** elems = (KlAst**)malloc(sizeof (KlAst*));
  if (kl_unlikely(!elems)) {
    klast_delete(expr);
    return klparser_error_oom(parser, lex);
  }
  elems[0] = expr;
  KlAstExprList* exprlist = klast_exprlist_create(elems, 1, expr->begin, expr->end);
  if (kl_unlikely(!exprlist))
    return klparser_error_oom(parser, lex);
  return exprlist;
}

static KlAst* klparser_exprunit(KlParser* parser, KlLex* lex, bool* inparenthesis) {
  if (inparenthesis) *inparenthesis = false;
  switch (kllex_tokkind(lex)) {
    case KLTK_INT: {
      KlAstConstant* ast = klast_constant_create_integer(lex->tok.intval, kllex_tokbegin(lex), kllex_tokend(lex));
      if (kl_unlikely(!ast)) return klparser_error_oom(parser, lex);
      kllex_next(lex);
      return klast(ast);
    }
    case KLTK_INTDOT: {
      KlAstConstant* ast = klast_constant_create_integer(lex->tok.intval, kllex_tokbegin(lex), kllex_tokend(lex) - 1);
      if (kl_unlikely(!ast)) return klparser_error_oom(parser, lex);
      kllex_setcurrtok(lex, KLTK_DOT, kllex_tokend(lex) - 1, kllex_tokend(lex));
      return klast(ast);
    }
    case KLTK_FLOAT: {
      KlAstConstant* ast = klast_constant_create_float(lex->tok.floatval, kllex_tokbegin(lex), kllex_tokend(lex));
      if (kl_unlikely(!ast)) return klparser_error_oom(parser, lex);
      kllex_next(lex);
      return klast(ast);
    }
    case KLTK_STRING: {
      KlAstConstant* ast = klast_constant_create_string(lex->tok.string, kllex_tokbegin(lex), kllex_tokend(lex));
      if (kl_unlikely(!ast)) return klparser_error_oom(parser, lex);
      kllex_next(lex);
      return klast(ast);
    }
    case KLTK_BOOLVAL: {
      KlAstConstant* ast = klast_constant_create_boolean(lex->tok.boolval, kllex_tokbegin(lex), kllex_tokend(lex));
      if (kl_unlikely(!ast)) return klparser_error_oom(parser, lex);
      kllex_next(lex);
      return klast(ast);
    }
    case KLTK_NIL: {
      KlAstConstant* ast = klast_constant_create_nil(kllex_tokbegin(lex), kllex_tokend(lex));
      if (kl_unlikely(!ast)) return klparser_error_oom(parser, lex);
      kllex_next(lex);
      return klast(ast);
    }
    case KLTK_ID: {
      KlAstIdentifier* ast = klast_id_create(lex->tok.string, kllex_tokbegin(lex), kllex_tokend(lex));
      if (kl_unlikely(!ast)) return klparser_error_oom(parser, lex);
      kllex_next(lex);
      return klast(ast);
    }
    case KLTK_VARARG: {
      KlAstVararg* ast = klast_vararg_create(kllex_tokbegin(lex), kllex_tokend(lex));
      if (kl_unlikely(!ast)) return klparser_error_oom(parser, lex);
      kllex_next(lex);
      return klast(ast);
    }
    case KLTK_LBRACKET: {
      return klast(klparser_array(parser, lex));
    }
    case KLTK_LBRACE: {
      return klparser_exprbrace(parser, lex);
    }
    case KLTK_LPAREN: {
      KlFileOffset begin = kllex_tokbegin(lex);
      kllex_next(lex);
      KlFileOffset mayend = kllex_tokend(lex);
      if (kllex_trymatch(lex, KLTK_RPAREN)) { /* empty exprlist */
        KlAstExprList* exprlist = klparser_emptyexprlist(parser, lex, begin, mayend);
        return klast(exprlist);
      }
      KlAst* expr = klparser_expr(parser, lex);
      if (kllex_check(lex, KLTK_RPAREN)) {  /* not an exprlist */
        if (inparenthesis) *inparenthesis = true;
        klast_setposition(expr, begin, kllex_tokend(lex));
        kllex_next(lex);
        return expr;
      }
      /* is a exprlist */
      klparser_returnifnull(expr);
      KlAstExprList* exprlist = klparser_finishexprlist(parser, lex, expr);
      KlFileOffset end = kllex_tokend(lex);
      klparser_match(parser, lex, KLTK_RPAREN);
      klparser_returnifnull(exprlist);
      klast_setposition(exprlist, begin, end);
      return klast(exprlist);
    }
    case KLTK_NEW: {
      return klast(klparser_exprnew(parser, lex));
    }
    default: {
      klparser_error(parser, kllex_inputstream(lex), kllex_tokbegin(lex), kllex_tokend(lex),
                     "expected '(', '{', '[', true, false, identifier, string, new or integer");
      return NULL;
    }
  }
}

static KlAst* klparser_array(KlParser* parser, KlLex* lex) {
  kl_assert(kllex_check(lex, KLTK_LBRACKET), "expected \'[\'");
  KlFileOffset begin = kllex_tokbegin(lex);
  kllex_next(lex);  /* skip '[' */
  KlAstExprList* exprlist = klparser_exprlist_mayempty(parser, lex);
  if (kl_unlikely(!exprlist)) {
    klparser_discardto(lex, KLTK_RBRACKET);
    return NULL;
  }
  if (kllex_trymatch(lex, KLTK_BAR)) { /* array generator */
    KlAstIdentifier* arrid = klast_id_create(klparser_newtmpid(parser, lex), klast_begin(exprlist), klast_end(exprlist));
    klparser_oomifnull(arrid);
    KlAstPost* exprpush = klast_post_create(KLTK_APPEND, klast(arrid), klast(exprlist), klast_begin(exprlist), klast_end(exprlist));
    klparser_oomifnull(exprpush);
    KlAstExprList* single_expr = klparser_singletonexprlist(parser, lex, klast(exprpush));
    klparser_oomifnull(single_expr);
    KlAstStmtExpr* inner_stmt = klast_stmtexpr_create(single_expr, klast_begin(exprlist), klast_end(exprlist));
    klparser_oomifnull(inner_stmt);
    KlAstStmtList* stmtlist = klparser_generator(parser, lex, klcast(KlAst*, inner_stmt));
    KlFileOffset end = kllex_tokend(lex);
    klparser_match(parser, lex, KLTK_RBRACKET);
    klparser_returnifnull(stmtlist);  /* 'inner_stmt' will be deleted in klparser_generator() when error occurred */
    KlAstArrayGenerator* array = klast_arraygenerator_create(arrid->id, stmtlist, begin, end);
    return klast(array);
  } else {
    KlFileOffset end = kllex_tokend(lex);
    klparser_match(parser, lex, KLTK_RBRACKET);
    KlAstArray* array = klast_array_create(exprlist, begin, end);
    klparser_oomifnull(array);
    return klast(array);
  }
}

KlAstExprList* klparser_finishexprlist(KlParser* parser, KlLex* lex, KlAst* expr) {
  KArray exprarray;
  if (kl_unlikely(!karray_init(&exprarray)))
    return klparser_error_oom(parser, lex);
  if (kl_unlikely(!karray_push_back(&exprarray, expr))) {
    klparser_destroy_astarray(&exprarray);
    klast_delete(expr);
    return klparser_error_oom(parser, lex);
  }
  while (kllex_trymatch(lex, KLTK_COMMA) && klparser_exprbegin(lex)) {
    KlAst* expr = klparser_expr(parser, lex);
    if (kl_unlikely(!expr)) continue;
    klparser_karr_pushast(&exprarray, expr);
  }
  kl_assert(karray_size(&exprarray) != 0, "");
  karray_shrink(&exprarray);
  size_t nelem = karray_size(&exprarray);
  KlAst** exprs = (KlAst**)karray_steal(&exprarray);
  KlAstExprList* exprlist = klast_exprlist_create(exprs, nelem, expr->begin, klast_end(exprs[nelem - 1]));
  klparser_oomifnull(exprlist);
  return exprlist;
}

static KlAst* klparser_exprbrace(KlParser* parser, KlLex* lex) {
  kl_assert(kllex_check(lex, KLTK_LBRACE), "");
  KlFileOffset begin = kllex_tokbegin(lex);
  kllex_next(lex);
  KlAst* expr = klparser_exprbrace_inner(parser, lex);
  KlFileOffset end = kllex_tokend(lex);
  klparser_match(parser, lex, KLTK_RBRACE);
  klparser_returnifnull(expr);
  klast_setposition(expr, begin, end);
  return expr;
}

static KlAst* klparser_exprbrace_inner(KlParser* parser, KlLex* lex) {
  if (kllex_check(lex, KLTK_RBRACE) || kllex_check(lex, KLTK_LOCAL) || kllex_check(lex, KLTK_SHARED)) {  /* is class */
    return klast(klparser_finishclass(parser, lex, (KlStrDesc) { .id = 0, .length = 0 }, NULL, false));
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
    KlAst* expr = klparser_expr(parser, lex);
    return klast(klparser_finishclass(parser, lex, id, expr, true));
  } else if (kllex_trymatch(lex, KLTK_COLON)) { /* a map? */
    if (kl_unlikely(exprlist->nexpr != 1)) {
      klparser_error(parser, kllex_inputstream(lex),
                     klast_begin(exprlist), klast_end(exprlist),
                     "should be a single expression in map constructor");
    }
    KlAst* key = klast_exprlist_stealfirst_and_destroy(exprlist);
    KlAst* val = klparser_expr(parser, lex);
    return klparser_finishmap(parser, lex, key, val);
  } else {
    /* else is a generator */
    klparser_match(parser, lex, KLTK_BAR);
    KlAstYield* yieldexpr = klast_yield_create(exprlist, klast_begin(exprlist), klast_end(exprlist));
    klparser_oomifnull(yieldexpr);
    KlAstExprList* single_expr = klparser_singletonexprlist(parser, lex, klast(yieldexpr));
    klparser_oomifnull(single_expr);
    KlAstStmtExpr* stmtexpr = klast_stmtexpr_create(single_expr, klast_begin(yieldexpr),  klast_end(yieldexpr));
    klparser_oomifnull(stmtexpr);
    KlAstStmtList* generator = klparser_generator(parser, lex, klast(stmtexpr));
    klparser_returnifnull(generator);
    KlAstExprList* params = klparser_emptyexprlist(parser, lex, klast_begin(stmtexpr), klast_begin(stmtexpr));
    if (kl_unlikely(!params)) {
      klast_delete(generator);
      return NULL;
    }
    KlAstFunc* func = klast_func_create(generator, params, false, false, klast_begin(stmtexpr), klast_end(generator));
    klparser_oomifnull(func);
    KlAstPre* asyncexpr = klast_pre_create(KLTK_ASYNC, klast(func), KLPARSER_ERROR_PH_FILEPOS, KLPARSER_ERROR_PH_FILEPOS);
    klparser_oomifnull(asyncexpr);
    return klast(asyncexpr);
  }
}

static KlAst* klparser_finishmap(KlParser* parser, KlLex* lex, KlAst* firstkey, KlAst* firstval) {
  /* if firstkey is NULL, then the map is not preparsed */
  KArray keys;
  KArray vals;
  if (kl_unlikely(!karray_init(&keys) || !karray_init(&vals))) {
    karray_destroy(&keys);
    karray_destroy(&vals);
    return klparser_error_oom(parser, lex);
  }

  if (firstkey) {
    klparser_karr_pushast(&keys, firstkey);
    klparser_karr_pushast(&vals, firstval);
    kllex_trymatch(lex, KLTK_COMMA);
  }

  while (klparser_exprbegin(lex)) {
    KlAst* key = klparser_expr(parser, lex);
    klparser_match(parser, lex, KLTK_COLON);
    KlAst* val = klparser_expr(parser, lex);
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
  KlAst** keys_stolen = (KlAst**)karray_steal(&keys);
  KlAst** vals_stolen = (KlAst**)karray_steal(&vals);

  if (kllex_check(lex, KLTK_BAR)) { /* is map generator */
    return klast(klparser_finishmapgenerator(parser, lex, keys_stolen, vals_stolen, npair));
  } else {  /* a normal map */
    KlAstMap* map = klast_map_create(keys_stolen, vals_stolen, npair, KLPARSER_ERROR_PH_FILEPOS, KLPARSER_ERROR_PH_FILEPOS);
    klparser_oomifnull(map);
    return klast(map);
  }
}

static void klparser_mapgenerator_helper_clean(KlAst** keys, KlAst** vals, size_t npair) {
  for (size_t i = 0; i < npair; ++i) {
    if (keys[i]) klast_delete(keys[i]);
    if (vals[i]) klast_delete(vals[i]);
  }
  free(keys);
  free(vals);
}

static KlAstMapGenerator* klparser_finishmapgenerator(KlParser* parser, KlLex* lex, KlAst** keys, KlAst** vals, size_t npair) {
  kl_assert(kllex_check(lex, KLTK_BAR), "");
  KlFileOffset inner_begin = npair == 0 ? kllex_tokbegin(lex) : klast_begin(keys[0]);
  KlFileOffset inner_end = npair == 0 ? kllex_tokend(lex) : klast_end(vals[npair - 1]);
  kllex_next(lex);

  KlStrDesc tmpmapid = klparser_newtmpid(parser, lex);
  for (size_t i = 0; i < npair; ++i) {
    KlAstIdentifier* mapid = klast_id_create(tmpmapid, inner_begin, inner_begin);
    if (kl_unlikely(!mapid)) {
      klparser_mapgenerator_helper_clean(keys, vals, npair);
      return klparser_error_oom(parser, lex);
    }
    keys[i] = klast(klast_post_create(KLTK_INDEX, klast(mapid), keys[i], klast_begin(keys[i]), klast_end(keys[i])));
    if (kl_unlikely(!keys[i])) {
      klparser_mapgenerator_helper_clean(keys, vals, npair);
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

  KlAstStmtList* stmts = klparser_generator(parser, lex, klast(inner_stmt));
  klparser_returnifnull(stmts);
  KlAstMapGenerator* mapgen = klast_mapgenerator_create(tmpmapid, stmts, KLPARSER_ERROR_PH_FILEPOS, KLPARSER_ERROR_PH_FILEPOS);
  klparser_oomifnull(mapgen);
  return mapgen;
}

static KlAstClass* klparser_finishclass(KlParser* parser, KlLex* lex, KlStrDesc id, KlAst* expr, bool preparsed) {
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
    if (kl_unlikely(!karray_push_back(&vals, expr))) {
      if (expr) klast_delete(expr);
      klparser_error_oom(parser, lex);
    }
    if (kllex_trymatch(lex, KLTK_COMMA))  /* has trailing shared k-v pairs */
      klparser_sharedlist(parser, lex, &fields, &vals);
    kllex_trymatch(lex, KLTK_SEMI);
  }

  while (!kllex_check(lex, KLTK_RBRACE)) {
    if (kllex_check(lex, KLTK_LOCAL)) {
      kllex_next(lex);
      klparser_locallist(parser, lex, &fields, &vals);
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
  KlAstClass* klclass = klast_class_create(klcfd_steal(&fields), (KlAst**)karray_steal(&vals), nfield, NULL, KLPARSER_ERROR_PH_FILEPOS, KLPARSER_ERROR_PH_FILEPOS);
  klparser_oomifnull(klclass);
  return klclass;
}

static void klparser_locallist(KlParser* parser, KlLex* lex, KlCfdArray* fields, KArray* vals) {
  KlAstClassFieldDesc fielddesc;
  fielddesc.shared = false;
  do {
    if (kl_unlikely(!klparser_check(parser, lex, KLTK_ID)))
      return;
    fielddesc.name = lex->tok.string;
    if (kl_unlikely(!klcfd_push_back(fields, &fielddesc) || !karray_push_back(vals, NULL))) {
      klparser_error_oom(parser, lex);
    }
    kllex_next(lex);
  } while (kllex_trymatch(lex, KLTK_COMMA));
  return;
}

static void klparser_sharedlist(KlParser* parser, KlLex* lex, KlCfdArray* fields, KArray* vals) {
  KlAstClassFieldDesc fielddesc;
  fielddesc.shared = true;
  do {
    if (kl_unlikely(!klparser_check(parser, lex, KLTK_ID)))
      return;
    fielddesc.name = lex->tok.string;
    kllex_next(lex);
    klparser_match(parser, lex, KLTK_ASSIGN);
    KlAst* val = klparser_expr(parser, lex);
    if (kl_unlikely(!klcfd_push_back(fields, &fielddesc)))
      klparser_error_oom(parser, lex);
    klparser_karr_pushast(vals, val);
  } while (kllex_trymatch(lex, KLTK_COMMA));
  return;
}

static KlAst* klparser_dotchain(KlParser* parser, KlLex* lex) {
  KlAst* dotexpr = klparser_exprunit(parser, lex, NULL);
  while (kllex_trymatch(lex, KLTK_DOT)) {
    if (kl_unlikely(!klparser_check(parser, lex, KLTK_ID)))
      continue;
    if (kl_unlikely(!dotexpr)) {
      kllex_next(lex);
      continue;
    }
    KlAstDot* dot = klast_dot_create(dotexpr, lex->tok.string, dotexpr->begin, kllex_tokend(lex));
    klparser_oomifnull(dot);
    dotexpr = klast(dot);
    kllex_next(lex);
  }
  return dotexpr;
}

static KlAstNew* klparser_exprnew(KlParser* parser, KlLex* lex) {
  kl_assert(kllex_check(lex, KLTK_NEW), "");

  kllex_next(lex);
  KlAst* klclass = kllex_check(lex, KLTK_LPAREN)
                 ? klparser_exprunit(parser, lex, NULL)
                 : klparser_dotchain(parser, lex);
  if (kllex_trymatch(lex, KLTK_LPAREN)) { /* has initialization list */
    KlAstExprList* args = klparser_exprlist_mayempty(parser, lex);
    klparser_match(parser, lex, KLTK_RPAREN);
    if (kl_unlikely(!klclass || !args)) {
      if (klclass) klast_delete(klclass);
      if (args) klast_delete(args);
      return NULL;
    }
    KlAstNew* newexpr = klast_new_create(klclass, klcast(KlAstExprList*, args), klast_begin(klclass), klast_end(args));
    klparser_oomifnull(newexpr);
    return newexpr;
  } else {
    klparser_returnifnull(klclass);
    KlAstNew* newexpr = klast_new_create(klclass, NULL, klast_begin(klclass), klast_end(klclass));
    klparser_oomifnull(newexpr);
    return newexpr;
  }
}

static KlAst* klparser_exprpre(KlParser* parser, KlLex* lex) {
  switch (kllex_tokkind(lex)) {
    case KLTK_MINUS: {
      KlFileOffset begin = kllex_tokbegin(lex);
      kllex_next(lex);
      KlAst* expr = klparser_exprpre(parser, lex);
      klparser_returnifnull(expr);
      KlAstPre* neg = klast_pre_create(KLTK_MINUS, expr, begin, expr->end);
      klparser_oomifnull(neg);
      return klast(neg);
    }
    case KLTK_LEN: {
      KlFileOffset begin = kllex_tokbegin(lex);
      kllex_next(lex);
      KlAst* expr = klparser_exprpre(parser, lex);
      klparser_returnifnull(expr);
      KlAstPre* notexpr = klast_pre_create(KLTK_LEN, expr, begin, expr->end);
      klparser_oomifnull(notexpr);
      return klast(notexpr);
    }
    case KLTK_NOT: {
      KlFileOffset begin = kllex_tokbegin(lex);
      kllex_next(lex);
      KlAst* expr = klparser_exprpre(parser, lex);
      klparser_returnifnull(expr);
      KlAstPre* notexpr = klast_pre_create(KLTK_NOT, expr, begin, expr->end);
      klparser_oomifnull(notexpr);
      return klast(notexpr);
    }
    case KLTK_ARROW: {
      KlFileOffset begin = kllex_tokbegin(lex);
      KlAstStmtList* block = klparser_arrowfuncbody(parser, lex);
      klparser_returnifnull(block);
      KlAstExprList* params = klparser_emptyexprlist(parser, lex, begin, begin);
      if (kl_unlikely(!params)) {
        klast_delete(block);
        return NULL;
      }
      KlAstFunc* func = klast_func_create(block, params, false, false, begin, klast_end(block));
      klparser_oomifnull(func);
      return klast(func);
    }
    case KLTK_DARROW: {
      KlFileOffset begin = kllex_tokbegin(lex);
      KlAstStmtList* block = klparser_darrowfuncbody(parser, lex);
      klparser_returnifnull(block);
      KlAstExprList* params = klparser_emptyexprlist(parser, lex, begin, begin);
      if (kl_unlikely(!params)) {
        klast_delete(block);
        return NULL;
      }
      KlAstFunc* func = klast_func_create(block, params, false, false, begin, klast_end(block));
      klparser_oomifnull(func);
      return klast(func);
    }
    case KLTK_YIELD: {
      KlFileOffset begin = kllex_tokbegin(lex);
      kllex_next(lex);
      KlAstExprList* exprlist = klparser_exprlist_mayempty(parser, lex);
      klparser_returnifnull(exprlist);
      KlAstYield* yieldexpr = klast_yield_create(exprlist, begin, klast_end(exprlist));
      klparser_oomifnull(yieldexpr);
      return klast(yieldexpr);
    }
    case KLTK_ASYNC: {
      KlFileOffset begin = kllex_tokbegin(lex);
      kllex_next(lex);
      KlAst* expr = klparser_exprpre(parser, lex);
      klparser_returnifnull(expr);
      KlAstPre* asyncexpr = klast_pre_create(KLTK_ASYNC, expr, begin, expr->end);
      klparser_oomifnull(asyncexpr);
      return klast(asyncexpr);
    }
    case KLTK_INHERIT: {
      KlFileOffset begin = kllex_tokbegin(lex);
      kllex_next(lex);
      KlAst* base = klparser_exprpre(parser, lex);
      klparser_returnifnull(base);
      klparser_match(parser, lex, KLTK_COLON);
      KlAst* ast = klparser_exprunit(parser, lex, NULL);
      if (kl_unlikely(!ast)) {
        klast_delete(base);
        return NULL;
      }
      if (kl_unlikely(klast_kind(ast) != KLAST_EXPR_CLASS)) {
        klparser_error(parser, kllex_inputstream(lex), ast->begin, ast->end,
                       "must be a class definition");
        klast_delete(base);
        klast_delete(ast);
        return NULL;
      }
      KlAstClass* klclass = klcast(KlAstClass*, ast);
      klclass->baseclass = base;
      klast_setposition(klclass, begin, klast_end(klclass));
      return klast(klclass);
    }
    case KLTK_METHOD: {
      KlFileOffset begin = kllex_tokbegin(lex);
      kllex_next(lex);
      KlAst* expr = klparser_exprpre(parser, lex);
      klparser_returnifnull(expr);
      if (kl_unlikely(klast_kind(expr) != KLAST_EXPR_FUNC)) {
        klparser_error(parser, kllex_inputstream(lex), expr->begin, expr->end,
                       "'method' must be followed by a function construction");
        return expr;
      }
      KlAst* idthis = klast(klast_id_create(parser->string.this, klast_begin(expr), klast_begin(expr)));
      if (kl_unlikely(!idthis)) {
        klast_delete(expr);
        return klparser_error_oom(parser, lex);
      }
      KlAstFunc* func = klcast(KlAstFunc*, expr);
      KlAstExprList* funcparams = func->params;
      KlAst** newarr = (KlAst**)realloc(funcparams->exprs, (funcparams->nexpr + 1) * sizeof (KlAst*));
      if (kl_unlikely(!newarr)) {
        klast_delete(expr);
        klast_delete(idthis);
        return klparser_error_oom(parser, lex);
      }
      memmove(newarr + 1, newarr, funcparams->nexpr * sizeof (KlAst*));
      newarr[0] = idthis;   /* add a parameter named 'this' */
      funcparams->exprs = newarr;
      ++funcparams->nexpr;
      func->is_method = true;
      klast_setposition(func, begin, expr->end);
      return klast(func);
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

static KlAstExprList* klparser_correctfuncparams(KlParser* parser, KlLex* lex, KlAst* expr, bool* vararg) {
  *vararg = false;
  if (klast_kind(expr) == KLAST_EXPR_LIST) {
    KlAstExprList* params = klcast(KlAstExprList*, expr);
    KlAst** exprs = params->exprs;
    size_t nexpr = params->nexpr;
    for (size_t i = 0; i < nexpr; ++i) {
      if (klast_kind(exprs[i]) == KLAST_EXPR_VARARG) {
        if (kl_unlikely(i != nexpr - 1))
          klparser_error(parser, kllex_inputstream(lex), exprs[i]->begin, exprs[i]->end, "'...' can only appear at the end of the parameter list");
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
  if (kllex_check(lex, KLTK_LPAREN)) {
    KlFileOffset begin = kllex_tokbegin(lex);
    kllex_next(lex);
    KlAstExprList* exprlist = klparser_exprlist(parser, lex);
    KlFileOffset end = kllex_tokend(lex);
    klparser_match(parser, lex, KLTK_RPAREN);
    klparser_returnifnull(exprlist);
    KlAstStmtReturn* stmtreturn = klast_stmtreturn_create(exprlist, begin, end);
    klparser_oomifnull(stmtreturn);
    KlAst** stmts = (KlAst**)malloc(sizeof (KlAst**));
    if (kl_unlikely(!stmts)) {
      klast_delete(stmtreturn);
      return klparser_error_oom(parser, lex);
    }
    stmts[0] = klast(stmtreturn);
    KlAstStmtList* block = klast_stmtlist_create(stmts, 1, klast_begin(stmtreturn), klast_end(stmtreturn));
    klparser_oomifnull(block);
    return block;
  } else {
    klparser_check(parser, lex, KLTK_LBRACE);
    KlFileOffset begin = kllex_tokbegin(lex);
    kllex_next(lex);
    KlAstStmtList* block = klparser_stmtlist(parser, lex);
    KlFileOffset end = kllex_tokend(lex);
    klparser_match(parser, lex, KLTK_RBRACE);
    klparser_returnifnull(block);
    klast_setposition(block, begin, end);
    return block;
  }
}

static KlAstStmtList* klparser_arrowfuncbody(KlParser* parser, KlLex* lex) {
  kl_assert(kllex_check(lex, KLTK_ARROW), "");
  kllex_next(lex);
  KlAst* expr = klparser_expr(parser, lex);
  klparser_returnifnull(expr);
  KlAstExprList* retvals = klparser_singletonexprlist(parser, lex, expr);
  klparser_returnifnull(retvals);
  KlAstStmtReturn* stmtreturn = klast_stmtreturn_create(retvals, expr->begin, expr->end);
  klparser_oomifnull(stmtreturn);
  KlAst** stmts = (KlAst**)malloc(sizeof (KlAst**));
  if (kl_unlikely(!stmts)) {
    klast_delete(stmtreturn);
    return klparser_error_oom(parser, lex);
  }
  stmts[0] = klast(stmtreturn);
  KlAstStmtList* block = klast_stmtlist_create(stmts, 1, klast_begin(stmtreturn), klast_end(stmtreturn));
  klparser_oomifnull(block);
  return block;
}

static KlAstCall* klparser_exprcall(KlParser* parser, KlLex* lex, KlAst* callable) {
  bool is_singleton = kllex_hasleadingblank(lex);
  if (is_singleton) {
    KlAst* unit = klparser_exprunit(parser, lex, NULL);
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
    KlAstCall* call = klast_call_create(callable, params, callable->begin, klast_end(unit));
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
    KlAstCall* call = klast_call_create(callable, params, callable->begin, klast_end(params));
    klparser_oomifnull(call);
    return call;
  }
}

static KlAstWhere* klparser_exprfinishwhere(KlParser* parser, KlLex* lex, KlAst* expr) {
  kllex_next(lex);
  KlAstStmtList* block = klparser_stmtblock(parser, lex);
  KlAstWhere* exprwhere = klast_where_create(expr, block, klast_begin(expr), klast_end(block));
  klparser_oomifnull(exprwhere);
  return exprwhere;
}

static KlAstMatch* klparser_exprmatch(KlParser* parser, KlLex* lex) {
  kl_assert(kllex_check(lex, KLTK_CASE), "expected 'case'");
  KlFileOffset begin = kllex_tokbegin(lex);
  kllex_next(lex);
  KlAst* matchobj = klparser_expr(parser, lex);
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
    KlAst* pattern = klparser_expr(parser, lex);
    klparser_match(parser, lex, KLTK_ASSIGN);
    KlAst* expr = klparser_expr(parser, lex);
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
  KlAst** patterns_stolen = klcast(KlAst**, karray_steal(&patterns));
  KlAst** exprs_stolen = klcast(KlAst**, karray_steal(&exprs));
  KlAstMatch* matchast = klast_match_create(matchobj, patterns_stolen, exprs_stolen, npattern, begin, klast_end(exprs_stolen[npattern - 1]));
  klparser_oomifnull(matchast);
  return matchast;
}

static KlAst* klparser_exprpost(KlParser* parser, KlLex* lex) {
  bool inparenthesis;
  KlAst* postexpr = klparser_exprunit(parser, lex, &inparenthesis);
  while (true) {
    switch (kllex_tokkind(lex)) {
      case KLTK_LBRACKET: { /* index */
        if (kllex_hasleadingblank(lex)) {
          postexpr = klast(klparser_exprcall(parser, lex, postexpr));
          break;
        }
        kllex_next(lex);
        KlAst* expr = klparser_expr(parser, lex);
        KlFileOffset end = kllex_tokend(lex);
        klparser_match(parser, lex, KLTK_RBRACKET);
        if (kl_unlikely(!expr)) break;
        if (kl_unlikely(!postexpr)) {
          klast_delete(expr);
          break;
        }
        KlAstPost* index = klast_post_create(KLTK_INDEX, postexpr, expr, postexpr->begin, end);
        klparser_oomifnull(index);
        postexpr = klast(index);
        break;
      }
      case KLTK_DARROW: {
        postexpr = inparenthesis ? klast(klparser_singletonexprlist(parser, lex, postexpr)) : postexpr;
        KlAstStmtList* block = klparser_darrowfuncbody(parser, lex);
        if (kl_unlikely(!block)) break;
        if (kl_unlikely(!postexpr)) {
          klast_delete(block);
          break;
        }
        bool vararg;
        KlAstExprList* params = klparser_correctfuncparams(parser, lex, postexpr, &vararg);
        if (kl_unlikely(!params)) {
          klast_delete(block);
          return NULL;
        }
        KlAstFunc* func = klast_func_create(block, params, vararg, false, klast_begin(params), klast_end(block));
        klparser_oomifnull(func);
        postexpr = klast(func);
        break;
      }
      case KLTK_ARROW: {
        postexpr = inparenthesis ? klast(klparser_singletonexprlist(parser, lex, postexpr)) : postexpr;
        KlAstStmtList* block = klparser_arrowfuncbody(parser, lex);
        if (kl_unlikely(!block)) break;
        if (kl_unlikely(!postexpr)) {
          klast_delete(block);
          break;
        }
        bool vararg;
        KlAstExprList* params = klparser_correctfuncparams(parser, lex, postexpr, &vararg);
        if (kl_unlikely(!params)) {
          klast_delete(block);
          return NULL;
        }
        KlAstFunc* func = klast_func_create(block, params, vararg, false, klast_begin(params), klast_end(block));
        klparser_oomifnull(func);
        postexpr = klast(func);
        break;
      }
      case KLTK_DOT: {
        kllex_next(lex);
        if (kl_unlikely(!klparser_check(parser, lex, KLTK_ID) || !postexpr))
          break;
        KlAstDot* dot = klast_dot_create(postexpr, lex->tok.string, postexpr->begin, kllex_tokend(lex));
        klparser_oomifnull(dot);
        postexpr = klast(dot);
        kllex_next(lex);
        break;
      }
      case KLTK_APPEND: {
        kllex_next(lex);
        KlAstExprList* exprlist = klparser_exprlist(parser, lex);
        if (kl_unlikely(!postexpr || !exprlist)) {
          if (exprlist) klast_delete(exprlist);
          break;
        }
        KlAstPost* arrpush = klast_post_create(KLTK_APPEND, postexpr, klast(exprlist), postexpr->begin, klast_end(exprlist));
        klparser_oomifnull(arrpush);
        postexpr = klast(arrpush);
        break;
      }
      case KLTK_ID:
      case KLTK_STRING:
      case KLTK_INT:
      case KLTK_FLOAT:
      case KLTK_BOOLVAL:
      case KLTK_NIL:
      case KLTK_VARARG:
      case KLTK_LPAREN:
      case KLTK_LBRACE: { /* function call */
        postexpr = klast(klparser_exprcall(parser, lex, postexpr));
        break;
      }
      default: {  /* no more postfix, just return */
        return postexpr;
      }
    }
  }
}

static KlAst* klparser_exprbin(KlParser* parser, KlLex* lex, int prio) {
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
  KlAst* left = klparser_exprpre(parser, lex);
  KlTokenKind op = kllex_tokkind(lex);
  while (kltoken_isbinop(op) && binop_prio[op] > prio) {
    kllex_next(lex);
    KlAst* right = klparser_exprbin(parser, lex, binop_prio[op]);
    if (kl_unlikely(!left || !right)) {
      if (left) klast_delete(left);
      if (right) klast_delete(right);
      return NULL;
    }
    KlAstBin* binexpr = klast_bin_create(op, left, right, left->begin, right->end);
    klparser_oomifnull(binexpr);
    left = klast(binexpr);
    op = kllex_tokkind(lex);
  }
  return klast(left);
}

