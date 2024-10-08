#include "include/ast/klast.h"
#include <stdbool.h>

static void klast_id_destroy(KlAstIdentifier* astid);
static void klast_map_destroy(KlAstMap* astmap);
static void klast_tuple_destroy(KlAstTuple* asttuple);
static void klast_mapcomprehension_destroy(KlAstMapComprehension* astmapcomprehension);
static void klast_array_destroy(KlAstArray* astarray);
static void klast_arraycomprehension_destroy(KlAstArrayComprehension* astarraycomprehension);
static void klast_class_destroy(KlAstClass* astclass);
static void klast_constant_destroy(KlAstConstant* astconstant);
static void klast_wildcard_destroy(KlAstWildcard* astwildcard);
static void klast_vararg_destroy(KlAstVararg* astvararg);
static void klast_exprlist_destroy(KlAstExprList* astexprlist);
static void klast_bin_destroy(KlAstBin* astbin);
static void klast_walrus_destroy(KlAstWalrus* astwalrus);
static void klast_async_destroy(KlAstAsync* astasync);
static void klast_pre_destroy(KlAstPre* astpre);
static void klast_new_destroy(KlAstNew* astnew);
static void klast_yield_destroy(KlAstYield* astyield);
static void klast_index_destroy(KlAstIndex* astpost);
static void klast_append_destroy(KlAstAppend* astappend);
static void klast_call_destroy(KlAstCall* astcall);
static void klast_dot_destroy(KlAstDot* astdot);
static void klast_func_destroy(KlAstFunc* astfunc);
static void klast_match_destroy(KlAstMatch* astmatch);
static void klast_where_destroy(KlAstWhere* astwhere);

static const KlAstInfo klast_id_vfunc = { .destructor = (KlAstDelete)klast_id_destroy, .kind = KLAST_EXPR_ID };
static const KlAstInfo klast_map_vfunc = { .destructor = (KlAstDelete)klast_map_destroy, .kind = KLAST_EXPR_MAP };
static const KlAstInfo klast_tuple_vfunc = { .destructor = (KlAstDelete)klast_tuple_destroy, .kind = KLAST_EXPR_TUPLE };
static const KlAstInfo klast_mapcomprehension_vfunc = { .destructor = (KlAstDelete)klast_mapcomprehension_destroy, .kind = KLAST_EXPR_MAPGEN };
static const KlAstInfo klast_array_vfunc = { .destructor = (KlAstDelete)klast_array_destroy, .kind = KLAST_EXPR_ARRAY };
static const KlAstInfo klast_arraycomprehension_vfunc = { .destructor = (KlAstDelete)klast_arraycomprehension_destroy, .kind = KLAST_EXPR_ARRGEN };
static const KlAstInfo klast_class_vfunc = { .destructor = (KlAstDelete)klast_class_destroy, .kind = KLAST_EXPR_CLASS };
static const KlAstInfo klast_constant_vfunc = { .destructor = (KlAstDelete)klast_constant_destroy, .kind = KLAST_EXPR_CONSTANT };
static const KlAstInfo klast_wildcard_vfunc = { .destructor = (KlAstDelete)klast_wildcard_destroy, .kind = KLAST_EXPR_WILDCARD };
static const KlAstInfo klast_vararg_vfunc = { .destructor = (KlAstDelete)klast_vararg_destroy, .kind = KLAST_EXPR_VARARG };
static const KlAstInfo klast_exprlist_vfunc = { .destructor = (KlAstDelete)klast_exprlist_destroy, .kind = KLAST_EXPR_LIST };
static const KlAstInfo klast_bin_vfunc = { .destructor = (KlAstDelete)klast_bin_destroy, .kind = KLAST_EXPR_BIN };
static const KlAstInfo klast_walrus_vfunc = { .destructor = (KlAstDelete)klast_walrus_destroy, .kind = KLAST_EXPR_WALRUS };
static const KlAstInfo klast_async_vfunc = { .destructor = (KlAstDelete)klast_async_destroy, .kind = KLAST_EXPR_ASYNC };
static const KlAstInfo klast_pre_vfunc = { .destructor = (KlAstDelete)klast_pre_destroy, .kind = KLAST_EXPR_PRE };
static const KlAstInfo klast_new_vfunc = { .destructor = (KlAstDelete)klast_new_destroy, .kind = KLAST_EXPR_NEW };
static const KlAstInfo klast_yield_vfunc = { .destructor = (KlAstDelete)klast_yield_destroy, .kind = KLAST_EXPR_YIELD };
static const KlAstInfo klast_post_vfunc = { .destructor = (KlAstDelete)klast_index_destroy, .kind = KLAST_EXPR_INDEX };
static const KlAstInfo klast_append_vfunc = { .destructor = (KlAstDelete)klast_append_destroy, .kind = KLAST_EXPR_APPEND };
static const KlAstInfo klast_call_vfunc = { .destructor = (KlAstDelete)klast_call_destroy, .kind = KLAST_EXPR_CALL };
static const KlAstInfo klast_dot_vfunc = { .destructor = (KlAstDelete)klast_dot_destroy, .kind = KLAST_EXPR_DOT };
static const KlAstInfo klast_func_vfunc = { .destructor = (KlAstDelete)klast_func_destroy, .kind = KLAST_EXPR_FUNC };
static const KlAstInfo klast_match_vfunc = { .destructor = (KlAstDelete)klast_match_destroy, .kind = KLAST_EXPR_MATCH };
static const KlAstInfo klast_where_vfunc = { .destructor = (KlAstDelete)klast_where_destroy, .kind = KLAST_EXPR_WHERE };

KlAstIdentifier* klast_id_create(KlStrDesc id, KlFileOffset begin, KlFileOffset end) {
  KlAstIdentifier* astid = klast_alloc(KlAstIdentifier);
  if (kl_unlikely(!astid)) return NULL;
  astid->id = id;
  klast_setposition(astid, begin, end);
  klast_init(astid, &klast_id_vfunc);
  return astid;
}

KlAstMap* klast_map_create(KlAstExpr** keys, KlAstExpr** vals, size_t npair, KlFileOffset begin, KlFileOffset end) {
  KlAstMap* astmap = klast_alloc(KlAstMap);
  if (kl_unlikely(!astmap)) {
    for (size_t i = 0; i < npair; ++i) {
      klast_delete(keys[i]);
      klast_delete(vals[i]);
    }
    free(keys);
    free(vals);
    return NULL;
  }
  astmap->keys = keys;
  astmap->vals = vals;
  astmap->npair = npair;
  klast_setposition(astmap, begin, end);
  klast_init(astmap, &klast_map_vfunc);
  return astmap;
}

KlAstTuple* klast_tuple_create(KlAstExpr** vals, size_t nval, KlFileOffset begin, KlFileOffset end) {
  KlAstTuple* asttuple = klast_alloc(KlAstTuple);
  if (kl_unlikely(!asttuple)) {
    for (size_t i = 0; i < nval; ++i) {
      klast_delete(vals[i]);
    }
    free(vals);
    return NULL;
  }
  asttuple->vals = vals;
  asttuple->nval = nval;
  klast_setposition(asttuple, begin, end);
  klast_init(asttuple, &klast_tuple_vfunc);
  return asttuple;
}

KlAstMapComprehension* klast_mapcomprehension_create(KlStrDesc arrid, KlAstStmtList* block, KlFileOffset begin, KlFileOffset end) {
  KlAstMapComprehension* astmapcomprehension = klast_alloc(KlAstMapComprehension);
  if (kl_unlikely(!astmapcomprehension)) {
    klast_delete(block);
    return NULL;
  }
  astmapcomprehension->mapid = arrid;
  astmapcomprehension->block = block;
  klast_setposition(astmapcomprehension, begin, end);
  klast_init(astmapcomprehension, &klast_mapcomprehension_vfunc);
  return astmapcomprehension;
}

KlAstArray* klast_array_create(KlAstExprList* exprlist, KlFileOffset begin, KlFileOffset end) {
  KlAstArray* astarray = klast_alloc(KlAstArray);
  if (kl_unlikely(!astarray)) {
    klast_delete(exprlist);
    return NULL;
  }
  astarray->exprlist = exprlist;
  klast_setposition(astarray, begin, end);
  klast_init(astarray, &klast_array_vfunc);
  return astarray;
}

KlAstArrayComprehension* klast_arraycomprehension_create(KlStrDesc arrid, KlAstStmtList* block, KlFileOffset begin, KlFileOffset end) {
  KlAstArrayComprehension* astarraycomprehension = klast_alloc(KlAstArrayComprehension);
  if (kl_unlikely(!astarraycomprehension)) {
    klast_delete(block);
    return NULL;
  }
  astarraycomprehension->arrid = arrid;
  astarraycomprehension->block = block;
  klast_setposition(astarraycomprehension, begin, end);
  klast_init(astarraycomprehension, &klast_arraycomprehension_vfunc);
  return astarraycomprehension;
}

KlAstClass* klast_class_create(KlAstClassFieldDesc* fields, KlAstExpr** vals, size_t nfield, KlAstExpr* base, KlFileOffset begin, KlFileOffset end) {
  KlAstClass* astclass = klast_alloc(KlAstClass);
  if (kl_unlikely(!astclass)) {
    for (size_t i = 0; i < nfield; ++i) {
      klast_delete(vals[i]);
    }
    free(fields);
    free(vals);
    return NULL;
  }
  astclass->fields = fields;
  astclass->vals = vals;
  astclass->nfield = nfield;
  astclass->baseclass = base;
  klast_setposition(astclass, begin, end);
  klast_init(astclass, &klast_class_vfunc);
  return astclass;
}

KlAstConstant* klast_constant_create_string(KlStrDesc string, KlFileOffset begin, KlFileOffset end) {
  KlAstConstant* astconstant = klast_alloc(KlAstConstant);
  if (kl_unlikely(!astconstant)) return NULL;
  astconstant->con.string = string;
  astconstant->con.type = KLC_STRING;
  klast_setposition(astconstant, begin, end);
  klast_init(astconstant, &klast_constant_vfunc);
  return astconstant;
}

KlAstConstant* klast_constant_create_integer(KlCInt intval, KlFileOffset begin, KlFileOffset end) {
  KlAstConstant* astconstant = klast_alloc(KlAstConstant);
  if (kl_unlikely(!astconstant)) return NULL;
  astconstant->con.intval = intval;
  astconstant->con.type = KLC_INT;
  klast_setposition(astconstant, begin, end);
  klast_init(astconstant, &klast_constant_vfunc);
  return astconstant;
}

KlAstConstant* klast_constant_create_float(KlCFloat floatval, KlFileOffset begin, KlFileOffset end) {
  KlAstConstant* astconstant = klast_alloc(KlAstConstant);
  if (kl_unlikely(!astconstant)) return NULL;
  astconstant->con.floatval = floatval;
  astconstant->con.type = KLC_FLOAT;
  klast_setposition(astconstant, begin, end);
  klast_init(astconstant, &klast_constant_vfunc);
  return astconstant;
}

KlAstConstant* klast_constant_create_boolean(KlCBool boolval, KlFileOffset begin, KlFileOffset end) {
  KlAstConstant* astconstant = klast_alloc(KlAstConstant);
  if (kl_unlikely(!astconstant)) return NULL;
  astconstant->con.boolval = boolval;
  astconstant->con.type = KLC_BOOL;
  klast_setposition(astconstant, begin, end);
  klast_init(astconstant, &klast_constant_vfunc);
  return astconstant;
}

KlAstConstant* klast_constant_create_nil(KlFileOffset begin, KlFileOffset end) {
  KlAstConstant* astconstant = klast_alloc(KlAstConstant);
  if (kl_unlikely(!astconstant)) return NULL;
  astconstant->con.type = KLC_NIL;
  klast_setposition(astconstant, begin, end);
  klast_init(astconstant, &klast_constant_vfunc);
  return astconstant;
}

KlAstWildcard* klast_wildcard_create(KlFileOffset begin, KlFileOffset end) {
  KlAstWildcard* astwildcard = klast_alloc(KlAstWildcard);
  if (kl_unlikely(!astwildcard)) return NULL;
  klast_setposition(astwildcard, begin, end);
  klast_init(astwildcard, &klast_wildcard_vfunc);
  return astwildcard;
}

KlAstVararg* klast_vararg_create(KlFileOffset begin, KlFileOffset end) {
  KlAstVararg* astvararg = klast_alloc(KlAstVararg);
  if (kl_unlikely(!astvararg)) return NULL;
  klast_setposition(astvararg, begin, end);
  klast_init(astvararg, &klast_vararg_vfunc);
  return astvararg;
}

KlAstExprList* klast_exprlist_create(KlAstExpr** exprs, size_t nexpr, KlFileOffset begin, KlFileOffset end) {
  KlAstExprList* astexprlist = klast_alloc(KlAstExprList);
  if (kl_unlikely(!astexprlist)) {
    for (size_t i = 0; i < nexpr; ++i) {
      klast_delete(exprs[i]);
    }
    free(exprs);
    return NULL;
  }
  astexprlist->exprs = exprs;
  astexprlist->nexpr = nexpr;
  klast_setposition(astexprlist, begin, end);
  klast_init(astexprlist, &klast_exprlist_vfunc);
  return astexprlist;
}

KlAstBin* klast_bin_create(KlTokenKind op, KlAstExpr* loperand, KlAstExpr* roperand, KlFileOffset begin, KlFileOffset end) {
  KlAstBin* astbin = klast_alloc(KlAstBin);
  if (kl_unlikely(!astbin)) {
    klast_delete(loperand);
    klast_delete(roperand);
    return NULL;
  }
  astbin->loperand = loperand;
  astbin->roperand = roperand;
  astbin->op = op;
  klast_setposition(astbin, begin, end);
  klast_init(astbin, &klast_bin_vfunc);
  return astbin;
}

KlAstWalrus* klast_walrus_create(KlAstExpr* pattern, KlAstExpr* rval, KlFileOffset begin, KlFileOffset end) {
  KlAstWalrus* astwalrus = klast_alloc(KlAstWalrus);
  if (kl_unlikely(!astwalrus)) {
    klast_delete(pattern);
    klast_delete(rval);
    return NULL;
  }
  astwalrus->pattern = pattern;
  astwalrus->rval = rval;
  klast_setposition(astwalrus, begin, end);
  klast_init(astwalrus, &klast_walrus_vfunc);
  return astwalrus;
}

KlAstAsync* klast_async_create(KlAstExpr* callable, KlFileOffset begin, KlFileOffset end) {
  KlAstAsync* astasync = klast_alloc(KlAstAsync);
  if (kl_unlikely(!astasync)) {
    klast_delete(callable);
    return NULL;
  }
  astasync->callable = callable;
  klast_setposition(astasync, begin, end);
  klast_init(astasync, &klast_async_vfunc);
  return astasync;
}

KlAstPre* klast_pre_create(KlTokenKind op, KlAstExpr* operand, KlFileOffset begin, KlFileOffset end) {
  KlAstPre* astpre = klast_alloc(KlAstPre);
  if (kl_unlikely(!astpre)) {
    klast_delete(operand);
    return NULL;
  }
  astpre->op = op;
  astpre->operand = operand;
  klast_setposition(astpre, begin, end);
  klast_init(astpre, &klast_pre_vfunc);
  return astpre;
}

KlAstNew* klast_new_create(KlAstExpr* klclass, KlAstExprList* args, KlFileOffset begin, KlFileOffset end) {
  KlAstNew* astnew = klast_alloc(KlAstNew);
  if (kl_unlikely(!astnew)) {
    klast_delete(klclass);
    if (args) klast_delete(args);
    return NULL;
  }
  astnew->klclass = klclass;
  astnew->args = args;
  klast_setposition(astnew, begin, end);
  klast_init(astnew, &klast_new_vfunc);
  return astnew;
}

KlAstYield* klast_yield_create(KlAstExprList* vals, KlFileOffset begin, KlFileOffset end) {
  KlAstYield* astyield = klast_alloc(KlAstYield);
  if (kl_unlikely(!astyield)) {
    klast_delete(vals);
    return NULL;
  }
  astyield->vals = vals;
  klast_setposition(astyield, begin, end);
  klast_init(astyield, &klast_yield_vfunc);
  return astyield;
}

KlAstIndex* klast_index_create(KlAstExpr* operand, KlAstExpr* index, KlFileOffset begin, KlFileOffset end) {
  KlAstIndex* astpost = klast_alloc(KlAstIndex);
  if (kl_unlikely(!astpost)) {
    klast_delete(operand);
    klast_delete(index);
    return NULL;
  }
  astpost->indexable = operand;
  astpost->index = index;
  klast_setposition(astpost, begin, end);
  klast_init(astpost, &klast_post_vfunc);
  return astpost;
}

KlAstAppend* klast_append_create(KlAstExpr* array, KlAstExprList* exprlist, KlFileOffset begin, KlFileOffset end) {
  KlAstAppend* astappend = klast_alloc(KlAstAppend);
  if (kl_unlikely(!astappend)) {
    klast_delete(array);
    klast_delete(exprlist);
    return NULL;
  }
  astappend->array = array;
  astappend->exprlist = exprlist;
  klast_setposition(astappend, begin, end);
  klast_init(astappend, &klast_append_vfunc);
  return astappend;
}

KlAstCall* klast_call_create(KlAstExpr* callable, KlAstExprList* args, KlFileOffset begin, KlFileOffset end) {
  KlAstCall* astcall = klast_alloc(KlAstCall);
  if (kl_unlikely(!astcall)) {
    klast_delete(callable);
    klast_delete(args);
    return NULL;
  }
  astcall->callable = callable;
  astcall->args = args;
  klast_setposition(astcall, begin, end);
  klast_init(astcall, &klast_call_vfunc);
  return astcall;
}

KlAstFunc* klast_func_create(KlAstStmtList* block, KlAstExprList* params, bool vararg, KlFileOffset begin, KlFileOffset end) {
  KlAstFunc* astfunc = klast_alloc(KlAstFunc);
  if (kl_unlikely(!astfunc)) {
    klast_delete(block);
    free(params);
    return NULL;
  }
  astfunc->block = block;
  astfunc->params = params;
  astfunc->vararg = vararg;
  klast_setposition(astfunc, begin, end);
  klast_init(astfunc, &klast_func_vfunc);
  return astfunc;
}

KlAstDot* klast_dot_create(KlAstExpr* operand, KlStrDesc field, KlFileOffset begin, KlFileOffset end) {
  KlAstDot* astdot = klast_alloc(KlAstDot);
  if (kl_unlikely(!astdot)) {
    klast_delete(operand);
    return NULL;
  }
  astdot->operand = operand;
  astdot->field = field;
  klast_setposition(astdot, begin, end);
  klast_init(astdot, &klast_dot_vfunc);
  return astdot;
}

KlAstMatch* klast_match_create(KlAstExpr* matchobj, KlAstExpr** patterns, KlAstExpr** exprs, size_t npattern, KlFileOffset begin, KlFileOffset end) {
  KlAstMatch* astmatch = klast_alloc(KlAstMatch);
  if (kl_unlikely(!astmatch)) {
    klast_delete(matchobj);
    for (size_t i = 0; i < npattern; ++i) {
      klast_delete(patterns[i]);
      klast_delete(exprs[i]);
    }
    free(patterns);
    free(exprs);
    return NULL;
  }
  astmatch->matchobj = matchobj;
  astmatch->patterns = patterns;
  astmatch->exprs = exprs;
  astmatch->npattern = npattern;
  klast_setposition(astmatch, begin, end);
  klast_init(astmatch, &klast_match_vfunc);
  return astmatch;
}

KlAstWhere* klast_where_create(KlAstExpr* expr, KlAstStmtList* block, KlFileOffset begin, KlFileOffset end) {
  KlAstWhere* astwhere = klast_alloc(KlAstWhere);
  if (kl_unlikely(!astwhere)) {
    klast_delete(expr);
    klast_delete(block);
    return NULL;
  }
  astwhere->expr = expr;
  astwhere->block = block;
  klast_setposition(astwhere, begin, end);
  klast_init(astwhere, &klast_where_vfunc);
  return astwhere;
}


KlAstExpr* klast_exprlist_stealfirst_and_destroy(KlAstExprList* exprlist) {
  kl_assert(exprlist->nexpr != 0, "");
  KlAstExpr* ret = exprlist->exprs[0];
  KlAstExpr** elems = exprlist->exprs;
  size_t nelem = exprlist->nexpr;
  for (size_t i = 1; i < nelem; ++i) {
    klast_delete(elems[i]);
  }
  free(elems);
  free(exprlist);
  return ret;
}


static void klast_id_destroy(KlAstIdentifier* astid) {
  (void)astid;
}

static void klast_map_destroy(KlAstMap* astmap) {
  KlAstExpr** keys = astmap->keys;
  KlAstExpr** vals = astmap->vals;
  size_t npair = astmap->npair;
  for (size_t i = 0; i < npair; ++i) {
    klast_delete(keys[i]);
    klast_delete(vals[i]);
  }
  free(keys);
  free(vals);
}

static void klast_tuple_destroy(KlAstTuple* asttuple) {
  KlAstExpr** vals = asttuple->vals;
  size_t nval = asttuple->nval;
  for (size_t i = 0; i < nval; ++i) {
    klast_delete(vals[i]);
  }
  free(vals);
}

static void klast_mapcomprehension_destroy(KlAstMapComprehension* astmapcomprehension) {
  klast_delete(astmapcomprehension->block);
}

static void klast_array_destroy(KlAstArray* astarray) {
  klast_delete(astarray->exprlist);
}

static void klast_arraycomprehension_destroy(KlAstArrayComprehension* astarraycomprehension) {
  klast_delete(astarraycomprehension->block);
}

static void klast_class_destroy(KlAstClass* astclass) {
  KlAstExpr** vals = astclass->vals;
  size_t nfield = astclass->nfield;
  for (size_t i = 0; i < nfield; ++i) {
    if (vals[i]) klast_delete(vals[i]);
  }
  if (astclass->baseclass)
    klast_delete(astclass->baseclass);
  free(vals);
  free(astclass->fields);
}

static void klast_constant_destroy(KlAstConstant* astconstant) {
  (void)astconstant;
}

static void klast_wildcard_destroy(KlAstWildcard* astwildcard) {
  (void)astwildcard;
}

static void klast_vararg_destroy(KlAstVararg* astvararg) {
  (void)astvararg;
}

static void klast_exprlist_destroy(KlAstExprList* astexprlist) {
  KlAstExpr** elems = astexprlist->exprs;
  size_t nelem = astexprlist->nexpr;
  for (size_t i = 0; i < nelem; ++i) {
    klast_delete(elems[i]);
  }
  free(elems);
}

static void klast_bin_destroy(KlAstBin* astbin) {
  klast_delete(astbin->loperand);
  klast_delete(astbin->roperand);
}

static void klast_walrus_destroy(KlAstWalrus* astwalrus) {
  klast_delete(astwalrus->pattern);
  klast_delete(astwalrus->rval);
}

static void klast_async_destroy(KlAstAsync* astasync) {
  klast_delete(astasync->callable);
}

static void klast_pre_destroy(KlAstPre* astpre) {
  klast_delete(astpre->operand);
}

static void klast_new_destroy(KlAstNew* astnew) {
  if (astnew->args) klast_delete(astnew->args);
  klast_delete(astnew->klclass);
}

static void klast_yield_destroy(KlAstYield* astyield) {
  klast_delete(astyield->vals);
}

static void klast_index_destroy(KlAstIndex* astpost) {
  klast_delete(astpost->indexable);
  klast_delete(astpost->index);
}

static void klast_append_destroy(KlAstAppend* astappend) {
  klast_delete(astappend->array);
  klast_delete(astappend->exprlist);
}

static void klast_call_destroy(KlAstCall* astcall) {
  klast_delete(astcall->callable);
  klast_delete(astcall->args);
}

static void klast_dot_destroy(KlAstDot* astdot) {
  klast_delete(astdot->operand);
}

static void klast_func_destroy(KlAstFunc* astfunc) {
  klast_delete(astfunc->params);
  klast_delete(astfunc->block);
}

static void klast_match_destroy(KlAstMatch* astmatch) {
  klast_delete(astmatch->matchobj);
  KlAstExpr** exprs = astmatch->exprs;
  KlAstExpr** patterns = astmatch->patterns;
  size_t npattern = astmatch->npattern;
  for (size_t i = 0; i < npattern; ++i) {
    klast_delete(patterns[i]);
    klast_delete(exprs[i]);
  }
  free(patterns);
  free(exprs);
}

static void klast_where_destroy(KlAstWhere* astwhere) {
  klast_delete(astwhere->expr);
  klast_delete(astwhere->block);
}
