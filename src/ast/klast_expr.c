#include "include/ast/klast.h"
#include <stdbool.h>

static void klast_id_destroy(KlAstIdentifier* astid);
static void klast_map_destroy(KlAstMap* astmap);
static void klast_array_destroy(KlAstArray* astarray);
static void klast_arraygenerator_destroy(KlAstArrayGenerator* astarraygenerator);
static void klast_class_destroy(KlAstClass* astclass);
static void klast_constant_destroy(KlAstConstant* astconstant);
static void klast_vararg_destroy(KlAstVararg* astvararg);
static void klast_exprlist_destroy(KlAstExprList* astexprlist);
static void klast_bin_destroy(KlAstBin* astbin);
static void klast_pre_destroy(KlAstPre* astpre);
static void klast_new_destroy(KlAstNew* astnew);
static void klast_yield_destroy(KlAstYield* astyield);
static void klast_post_destroy(KlAstPost* astpost);
static void klast_call_destroy(KlAstCall* astcall);
static void klast_dot_destroy(KlAstDot* astdot);
static void klast_func_destroy(KlAstFunc* astfunc);
static void klast_where_destroy(KlAstWhere* astwhere);

static KlAstInfo klast_id_vfunc = { .destructor = (KlAstDelete)klast_id_destroy, .kind = KLAST_EXPR_ID };
static KlAstInfo klast_map_vfunc = { .destructor = (KlAstDelete)klast_map_destroy, .kind = KLAST_EXPR_MAP };
static KlAstInfo klast_array_vfunc = { .destructor = (KlAstDelete)klast_array_destroy, .kind = KLAST_EXPR_ARR };
static KlAstInfo klast_arraygenerator_vfunc = { .destructor = (KlAstDelete)klast_arraygenerator_destroy, .kind = KLAST_EXPR_ARRGEN };
static KlAstInfo klast_class_vfunc = { .destructor = (KlAstDelete)klast_class_destroy, .kind = KLAST_EXPR_CLASS };
static KlAstInfo klast_constant_vfunc = { .destructor = (KlAstDelete)klast_constant_destroy, .kind = KLAST_EXPR_CONSTANT };
static KlAstInfo klast_vararg_vfunc = { .destructor = (KlAstDelete)klast_vararg_destroy, .kind = KLAST_EXPR_VARARG };
static KlAstInfo klast_exprlist_vfunc = { .destructor = (KlAstDelete)klast_exprlist_destroy, .kind = KLAST_EXPR_LIST };
static KlAstInfo klast_bin_vfunc = { .destructor = (KlAstDelete)klast_bin_destroy, .kind = KLAST_EXPR_BIN };
static KlAstInfo klast_pre_vfunc = { .destructor = (KlAstDelete)klast_pre_destroy, .kind = KLAST_EXPR_PRE };
static KlAstInfo klast_new_vfunc = { .destructor = (KlAstDelete)klast_new_destroy, .kind = KLAST_EXPR_NEW };
static KlAstInfo klast_yield_vfunc = { .destructor = (KlAstDelete)klast_yield_destroy, .kind = KLAST_EXPR_YIELD };
static KlAstInfo klast_post_vfunc = { .destructor = (KlAstDelete)klast_post_destroy, .kind = KLAST_EXPR_POST };
static KlAstInfo klast_call_vfunc = { .destructor = (KlAstDelete)klast_call_destroy, .kind = KLAST_EXPR_CALL };
static KlAstInfo klast_dot_vfunc = { .destructor = (KlAstDelete)klast_dot_destroy, .kind = KLAST_EXPR_DOT };
static KlAstInfo klast_func_vfunc = { .destructor = (KlAstDelete)klast_func_destroy, .kind = KLAST_EXPR_FUNC };
static KlAstInfo klast_where_vfunc = { .destructor = (KlAstDelete)klast_where_destroy, .kind = KLAST_EXPR_WHERE };

KlAstIdentifier* klast_id_create(KlStrDesc id, KlFileOffset begin, KlFileOffset end) {
  KlAstIdentifier* astid = klast_alloc(KlAstIdentifier);
  if (kl_unlikely(!astid)) return NULL;
  astid->id = id;
  klast_setposition(astid, begin, end);
  klast_init(astid, &klast_id_vfunc);
  return astid;
}

KlAstMap* klast_map_create(KlAst** keys, KlAst** vals, size_t npair, KlFileOffset begin, KlFileOffset end) {
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

KlAstArrayGenerator* klast_arraygenerator_create(KlStrDesc arrid, KlAstStmtList* block, KlFileOffset begin, KlFileOffset end) {
  KlAstArrayGenerator* astarraygenerator = klast_alloc(KlAstArrayGenerator);
  if (kl_unlikely(!astarraygenerator)) {
      klast_delete(block);
    return NULL;
  }
  astarraygenerator->arrid = arrid;
  astarraygenerator->block = block;
  klast_setposition(astarraygenerator, begin, end);
  klast_init(astarraygenerator, &klast_arraygenerator_vfunc);
  return astarraygenerator;
}

KlAstClass* klast_class_create(KlAstClassFieldDesc* fields, KlAst** vals, size_t nfield, KlAst* base, KlFileOffset begin, KlFileOffset end) {
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

KlAstVararg* klast_vararg_create(KlFileOffset begin, KlFileOffset end) {
  KlAstVararg* astvararg = klast_alloc(KlAstVararg);
  if (kl_unlikely(!astvararg)) return NULL;
  klast_setposition(astvararg, begin, end);
  klast_init(astvararg, &klast_vararg_vfunc);
  return astvararg;
}

KlAstExprList* klast_exprlist_create(KlAst** exprs, size_t nexpr, KlFileOffset begin, KlFileOffset end) {
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

KlAstBin* klast_bin_create(KlTokenKind op, KlAst* loperand, KlAst* roperand, KlFileOffset begin, KlFileOffset end) {
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

KlAstPre* klast_pre_create(KlTokenKind op, KlAst* operand, KlFileOffset begin, KlFileOffset end) {
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

KlAstNew* klast_new_create(KlAst* klclass, KlAstExprList* args, KlFileOffset begin, KlFileOffset end) {
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

KlAstPost* klast_post_create(KlTokenKind op, KlAst* operand, KlAst* post, KlFileOffset begin, KlFileOffset end) {
  KlAstPost* astpost = klast_alloc(KlAstPost);
  if (kl_unlikely(!astpost)) {
    klast_delete(operand);
    klast_delete(post);
    return NULL;
  }
  astpost->operand = operand;
  astpost->post = post;
  astpost->op = op;
  klast_setposition(astpost, begin, end);
  klast_init(astpost, &klast_post_vfunc);
  return astpost;
}

KlAstCall* klast_call_create(KlAst* callable, KlAstExprList* args, KlFileOffset begin, KlFileOffset end) {
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

KlAstFunc* klast_func_create(KlAstStmtList* block, KlAstExprList* params, bool vararg, bool is_method, KlFileOffset begin, KlFileOffset end) {
  KlAstFunc* astfunc = klast_alloc(KlAstFunc);
  if (kl_unlikely(!astfunc)) {
    klast_delete(block);
    free(params);
    return NULL;
  }
  astfunc->block = block;
  astfunc->params = params;
  astfunc->vararg = vararg;
  astfunc->is_method = is_method;
  klast_setposition(astfunc, begin, end);
  klast_init(astfunc, &klast_func_vfunc);
  return astfunc;
}

KlAstDot* klast_dot_create(KlAst* operand, KlStrDesc field, KlFileOffset begin, KlFileOffset end) {
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

KlAstWhere* klast_where_create(KlAst* expr, KlAstStmtList* block, KlFileOffset begin, KlFileOffset end) {
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



static void klast_id_destroy(KlAstIdentifier* astid) {
  (void)astid;
}

static void klast_map_destroy(KlAstMap* astmap) {
  KlAst** keys = astmap->keys;
  KlAst** vals = astmap->vals;
  size_t npair = astmap->npair;
  for (size_t i = 0; i < npair; ++i) {
    klast_delete(keys[i]);
    klast_delete(vals[i]);
  }
  free(keys);
  free(vals);
}

static void klast_array_destroy(KlAstArray* astarray) {
  klast_delete(astarray->exprlist);
}

static void klast_arraygenerator_destroy(KlAstArrayGenerator* astarraygenerator) {
  klast_delete(astarraygenerator->block);
}

static void klast_class_destroy(KlAstClass* astclass) {
  KlAst** vals = astclass->vals;
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

static void klast_vararg_destroy(KlAstVararg* astvararg) {
  (void)astvararg;
}

static void klast_exprlist_destroy(KlAstExprList* astexprlist) {
  KlAst** elems = astexprlist->exprs;
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

static void klast_post_destroy(KlAstPost* astpost) {
  klast_delete(astpost->operand);
  klast_delete(astpost->post);
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

static void klast_where_destroy(KlAstWhere* astwhere) {
  klast_delete(astwhere->expr);
  klast_delete(astwhere->block);
}
