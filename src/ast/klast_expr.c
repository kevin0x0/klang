#include "include/ast/klast.h"
#include <stdbool.h>

static void id_destroy(KlAstIdentifier* astid);
static void map_destroy(KlAstMap* astmap);
static void tuple_destroy(KlAstTuple* asttuple);
static void mapcomprehension_destroy(KlAstMapComprehension* astmapcomprehension);
static void array_destroy(KlAstArray* astarray);
static void arraycomprehension_destroy(KlAstArrayComprehension* astarraycomprehension);
static void class_destroy(KlAstClass* astclass);
static void constant_destroy(KlAstConstant* astconstant);
static void wildcard_destroy(KlAstWildcard* astwildcard);
static void vararg_destroy(KlAstVararg* astvararg);
static void exprlist_destroy(KlAstExprList* astexprlist);
static void bin_destroy(KlAstBin* astbin);
static void walrus_destroy(KlAstWalrus* astwalrus);
static void async_destroy(KlAstAsync* astasync);
static void pre_destroy(KlAstPre* astpre);
static void new_destroy(KlAstNew* astnew);
static void yield_destroy(KlAstYield* astyield);
static void index_destroy(KlAstIndex* astpost);
static void append_destroy(KlAstAppend* astappend);
static void call_destroy(KlAstCall* astcall);
static void dot_destroy(KlAstDot* astdot);
static void func_destroy(KlAstFunc* astfunc);
static void match_destroy(KlAstMatch* astmatch);
static void where_destroy(KlAstWhere* astwhere);

static const KlAstInfo id_vfunc = { .destructor = (KlAstDelete)id_destroy, .kind = KLAST_EXPR_ID };
static const KlAstInfo map_vfunc = { .destructor = (KlAstDelete)map_destroy, .kind = KLAST_EXPR_MAP };
static const KlAstInfo tuple_vfunc = { .destructor = (KlAstDelete)tuple_destroy, .kind = KLAST_EXPR_TUPLE };
static const KlAstInfo mapcomprehension_vfunc = { .destructor = (KlAstDelete)mapcomprehension_destroy, .kind = KLAST_EXPR_MAPGEN };
static const KlAstInfo array_vfunc = { .destructor = (KlAstDelete)array_destroy, .kind = KLAST_EXPR_ARRAY };
static const KlAstInfo arraycomprehension_vfunc = { .destructor = (KlAstDelete)arraycomprehension_destroy, .kind = KLAST_EXPR_ARRGEN };
static const KlAstInfo class_vfunc = { .destructor = (KlAstDelete)class_destroy, .kind = KLAST_EXPR_CLASS };
static const KlAstInfo constant_vfunc = { .destructor = (KlAstDelete)constant_destroy, .kind = KLAST_EXPR_CONSTANT };
static const KlAstInfo wildcard_vfunc = { .destructor = (KlAstDelete)wildcard_destroy, .kind = KLAST_EXPR_WILDCARD };
static const KlAstInfo vararg_vfunc = { .destructor = (KlAstDelete)vararg_destroy, .kind = KLAST_EXPR_VARARG };
static const KlAstInfo exprlist_vfunc = { .destructor = (KlAstDelete)exprlist_destroy, .kind = KLAST_EXPR_LIST };
static const KlAstInfo bin_vfunc = { .destructor = (KlAstDelete)bin_destroy, .kind = KLAST_EXPR_BIN };
static const KlAstInfo walrus_vfunc = { .destructor = (KlAstDelete)walrus_destroy, .kind = KLAST_EXPR_WALRUS };
static const KlAstInfo async_vfunc = { .destructor = (KlAstDelete)async_destroy, .kind = KLAST_EXPR_ASYNC };
static const KlAstInfo pre_vfunc = { .destructor = (KlAstDelete)pre_destroy, .kind = KLAST_EXPR_PRE };
static const KlAstInfo new_vfunc = { .destructor = (KlAstDelete)new_destroy, .kind = KLAST_EXPR_NEW };
static const KlAstInfo yield_vfunc = { .destructor = (KlAstDelete)yield_destroy, .kind = KLAST_EXPR_YIELD };
static const KlAstInfo post_vfunc = { .destructor = (KlAstDelete)index_destroy, .kind = KLAST_EXPR_INDEX };
static const KlAstInfo append_vfunc = { .destructor = (KlAstDelete)append_destroy, .kind = KLAST_EXPR_APPEND };
static const KlAstInfo call_vfunc = { .destructor = (KlAstDelete)call_destroy, .kind = KLAST_EXPR_CALL };
static const KlAstInfo dot_vfunc = { .destructor = (KlAstDelete)dot_destroy, .kind = KLAST_EXPR_DOT };
static const KlAstInfo func_vfunc = { .destructor = (KlAstDelete)func_destroy, .kind = KLAST_EXPR_FUNC };
static const KlAstInfo match_vfunc = { .destructor = (KlAstDelete)match_destroy, .kind = KLAST_EXPR_MATCH };
static const KlAstInfo where_vfunc = { .destructor = (KlAstDelete)where_destroy, .kind = KLAST_EXPR_WHERE };

KlAstIdentifier* klast_id_create(KlStrDesc id, KlFileOffset begin, KlFileOffset end) {
  KlAstIdentifier* astid = klast_alloc(KlAstIdentifier);
  if (kl_unlikely(!astid)) return NULL;
  astid->id = id;
  klast_setposition(astid, begin, end);
  klast_init(astid, &id_vfunc);
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
  klast_init(astmap, &map_vfunc);
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
  klast_init(asttuple, &tuple_vfunc);
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
  klast_init(astmapcomprehension, &mapcomprehension_vfunc);
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
  klast_init(astarray, &array_vfunc);
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
  klast_init(astarraycomprehension, &arraycomprehension_vfunc);
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
  klast_init(astclass, &class_vfunc);
  return astclass;
}

KlAstConstant* klast_constant_create_string(KlStrDesc string, KlFileOffset begin, KlFileOffset end) {
  KlAstConstant* astconstant = klast_alloc(KlAstConstant);
  if (kl_unlikely(!astconstant)) return NULL;
  astconstant->con.string = string;
  astconstant->con.type = KLC_STRING;
  klast_setposition(astconstant, begin, end);
  klast_init(astconstant, &constant_vfunc);
  return astconstant;
}

KlAstConstant* klast_constant_create_integer(KlCInt intval, KlFileOffset begin, KlFileOffset end) {
  KlAstConstant* astconstant = klast_alloc(KlAstConstant);
  if (kl_unlikely(!astconstant)) return NULL;
  astconstant->con.intval = intval;
  astconstant->con.type = KLC_INT;
  klast_setposition(astconstant, begin, end);
  klast_init(astconstant, &constant_vfunc);
  return astconstant;
}

KlAstConstant* klast_constant_create_float(KlCFloat floatval, KlFileOffset begin, KlFileOffset end) {
  KlAstConstant* astconstant = klast_alloc(KlAstConstant);
  if (kl_unlikely(!astconstant)) return NULL;
  astconstant->con.floatval = floatval;
  astconstant->con.type = KLC_FLOAT;
  klast_setposition(astconstant, begin, end);
  klast_init(astconstant, &constant_vfunc);
  return astconstant;
}

KlAstConstant* klast_constant_create_boolean(KlCBool boolval, KlFileOffset begin, KlFileOffset end) {
  KlAstConstant* astconstant = klast_alloc(KlAstConstant);
  if (kl_unlikely(!astconstant)) return NULL;
  astconstant->con.boolval = boolval;
  astconstant->con.type = KLC_BOOL;
  klast_setposition(astconstant, begin, end);
  klast_init(astconstant, &constant_vfunc);
  return astconstant;
}

KlAstConstant* klast_constant_create_nil(KlFileOffset begin, KlFileOffset end) {
  KlAstConstant* astconstant = klast_alloc(KlAstConstant);
  if (kl_unlikely(!astconstant)) return NULL;
  astconstant->con.type = KLC_NIL;
  klast_setposition(astconstant, begin, end);
  klast_init(astconstant, &constant_vfunc);
  return astconstant;
}

KlAstWildcard* klast_wildcard_create(KlFileOffset begin, KlFileOffset end) {
  KlAstWildcard* astwildcard = klast_alloc(KlAstWildcard);
  if (kl_unlikely(!astwildcard)) return NULL;
  klast_setposition(astwildcard, begin, end);
  klast_init(astwildcard, &wildcard_vfunc);
  return astwildcard;
}

KlAstVararg* klast_vararg_create(KlFileOffset begin, KlFileOffset end) {
  KlAstVararg* astvararg = klast_alloc(KlAstVararg);
  if (kl_unlikely(!astvararg)) return NULL;
  klast_setposition(astvararg, begin, end);
  klast_init(astvararg, &vararg_vfunc);
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
  klast_init(astexprlist, &exprlist_vfunc);
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
  klast_init(astbin, &bin_vfunc);
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
  klast_init(astwalrus, &walrus_vfunc);
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
  klast_init(astasync, &async_vfunc);
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
  klast_init(astpre, &pre_vfunc);
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
  klast_init(astnew, &new_vfunc);
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
  klast_init(astyield, &yield_vfunc);
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
  klast_init(astpost, &post_vfunc);
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
  klast_init(astappend, &append_vfunc);
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
  klast_init(astcall, &call_vfunc);
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
  klast_init(astfunc, &func_vfunc);
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
  klast_init(astdot, &dot_vfunc);
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
  klast_init(astmatch, &match_vfunc);
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
  klast_init(astwhere, &where_vfunc);
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


static void id_destroy(KlAstIdentifier* astid) {
  (void)astid;
}

static void map_destroy(KlAstMap* astmap) {
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

static void tuple_destroy(KlAstTuple* asttuple) {
  KlAstExpr** vals = asttuple->vals;
  size_t nval = asttuple->nval;
  for (size_t i = 0; i < nval; ++i) {
    klast_delete(vals[i]);
  }
  free(vals);
}

static void mapcomprehension_destroy(KlAstMapComprehension* astmapcomprehension) {
  klast_delete(astmapcomprehension->block);
}

static void array_destroy(KlAstArray* astarray) {
  klast_delete(astarray->exprlist);
}

static void arraycomprehension_destroy(KlAstArrayComprehension* astarraycomprehension) {
  klast_delete(astarraycomprehension->block);
}

static void class_destroy(KlAstClass* astclass) {
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

static void constant_destroy(KlAstConstant* astconstant) {
  (void)astconstant;
}

static void wildcard_destroy(KlAstWildcard* astwildcard) {
  (void)astwildcard;
}

static void vararg_destroy(KlAstVararg* astvararg) {
  (void)astvararg;
}

static void exprlist_destroy(KlAstExprList* astexprlist) {
  KlAstExpr** elems = astexprlist->exprs;
  size_t nelem = astexprlist->nexpr;
  for (size_t i = 0; i < nelem; ++i) {
    klast_delete(elems[i]);
  }
  free(elems);
}

static void bin_destroy(KlAstBin* astbin) {
  klast_delete(astbin->loperand);
  klast_delete(astbin->roperand);
}

static void walrus_destroy(KlAstWalrus* astwalrus) {
  klast_delete(astwalrus->pattern);
  klast_delete(astwalrus->rval);
}

static void async_destroy(KlAstAsync* astasync) {
  klast_delete(astasync->callable);
}

static void pre_destroy(KlAstPre* astpre) {
  klast_delete(astpre->operand);
}

static void new_destroy(KlAstNew* astnew) {
  if (astnew->args) klast_delete(astnew->args);
  klast_delete(astnew->klclass);
}

static void yield_destroy(KlAstYield* astyield) {
  klast_delete(astyield->vals);
}

static void index_destroy(KlAstIndex* astpost) {
  klast_delete(astpost->indexable);
  klast_delete(astpost->index);
}

static void append_destroy(KlAstAppend* astappend) {
  klast_delete(astappend->array);
  klast_delete(astappend->exprlist);
}

static void call_destroy(KlAstCall* astcall) {
  klast_delete(astcall->callable);
  klast_delete(astcall->args);
}

static void dot_destroy(KlAstDot* astdot) {
  klast_delete(astdot->operand);
}

static void func_destroy(KlAstFunc* astfunc) {
  klast_delete(astfunc->params);
  klast_delete(astfunc->block);
}

static void match_destroy(KlAstMatch* astmatch) {
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

static void where_destroy(KlAstWhere* astwhere) {
  klast_delete(astwhere->expr);
  klast_delete(astwhere->block);
}
