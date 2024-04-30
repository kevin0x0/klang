#include "include/cst/klcst_expr.h"
#include "include/cst/klcst.h"
#include <stdbool.h>

static void klcst_id_destroy(KlCstIdentifier* cstid);
static void klcst_map_destroy(KlCstMap* cstmap);
static void klcst_array_destroy(KlCstArray* cstarray);
static void klcst_arraygenerator_destroy(KlCstArrayGenerator* cstarraygenerator);
static void klcst_class_destroy(KlCstClass* cstclass);
static void klcst_constant_destroy(KlCstConstant* cstconstant);
static void klcst_vararg_destroy(KlCstVararg* cstvararg);
static void klcst_tuple_destroy(KlCstTuple* csttuple);
static void klcst_bin_destroy(KlCstBin* cstbin);
static void klcst_pre_destroy(KlCstPre* cstpre);
static void klcst_new_destroy(KlCstNew* cstnew);
static void klcst_yield_destroy(KlCstYield* cstyield);
static void klcst_post_destroy(KlCstPost* cstpost);
static void klcst_call_destroy(KlCstCall* cstcall);
static void klcst_dot_destroy(KlCstDot* cstdot);
static void klcst_func_destroy(KlCstFunc* cstfunc);
static void klcst_where_destroy(KlCstWhere* cstwhere);

static KlCstInfo klcst_id_vfunc = { .destructor = (KlCstDelete)klcst_id_destroy, .kind = KLCST_EXPR_ID };
static KlCstInfo klcst_map_vfunc = { .destructor = (KlCstDelete)klcst_map_destroy, .kind = KLCST_EXPR_MAP };
static KlCstInfo klcst_array_vfunc = { .destructor = (KlCstDelete)klcst_array_destroy, .kind = KLCST_EXPR_ARR };
static KlCstInfo klcst_arraygenerator_vfunc = { .destructor = (KlCstDelete)klcst_arraygenerator_destroy, .kind = KLCST_EXPR_ARRGEN };
static KlCstInfo klcst_class_vfunc = { .destructor = (KlCstDelete)klcst_class_destroy, .kind = KLCST_EXPR_CLASS };
static KlCstInfo klcst_constant_vfunc = { .destructor = (KlCstDelete)klcst_constant_destroy, .kind = KLCST_EXPR_CONSTANT };
static KlCstInfo klcst_vararg_vfunc = { .destructor = (KlCstDelete)klcst_vararg_destroy, .kind = KLCST_EXPR_VARARG };
static KlCstInfo klcst_tuple_vfunc = { .destructor = (KlCstDelete)klcst_tuple_destroy, .kind = KLCST_EXPR_TUPLE };
static KlCstInfo klcst_bin_vfunc = { .destructor = (KlCstDelete)klcst_bin_destroy, .kind = KLCST_EXPR_BIN };
static KlCstInfo klcst_pre_vfunc = { .destructor = (KlCstDelete)klcst_pre_destroy, .kind = KLCST_EXPR_PRE };
static KlCstInfo klcst_new_vfunc = { .destructor = (KlCstDelete)klcst_new_destroy, .kind = KLCST_EXPR_NEW };
static KlCstInfo klcst_yield_vfunc = { .destructor = (KlCstDelete)klcst_yield_destroy, .kind = KLCST_EXPR_YIELD };
static KlCstInfo klcst_post_vfunc = { .destructor = (KlCstDelete)klcst_post_destroy, .kind = KLCST_EXPR_POST };
static KlCstInfo klcst_call_vfunc = { .destructor = (KlCstDelete)klcst_call_destroy, .kind = KLCST_EXPR_CALL };
static KlCstInfo klcst_dot_vfunc = { .destructor = (KlCstDelete)klcst_dot_destroy, .kind = KLCST_EXPR_DOT };
static KlCstInfo klcst_func_vfunc = { .destructor = (KlCstDelete)klcst_func_destroy, .kind = KLCST_EXPR_FUNC };
static KlCstInfo klcst_where_vfunc = { .destructor = (KlCstDelete)klcst_where_destroy, .kind = KLCST_EXPR_WHERE };

KlCstIdentifier* klcst_id_create(KlStrDesc id, KlFileOffset begin, KlFileOffset end) {
  KlCstIdentifier* cstid = klcst_alloc(KlCstIdentifier);
  if (kl_unlikely(!cstid)) return NULL;
  cstid->id = id;
  klcst_setposition(cstid, begin, end);
  klcst_init(cstid, &klcst_id_vfunc);
  return cstid;
}

KlCstMap* klcst_map_create(KlCst** keys, KlCst** vals, size_t npair, KlFileOffset begin, KlFileOffset end) {
  KlCstMap* cstmap = klcst_alloc(KlCstMap);
  if (kl_unlikely(!cstmap)) {
    for (size_t i = 0; i < npair; ++i) {
      klcst_delete_raw(keys[i]);
      klcst_delete_raw(vals[i]);
    }
    free(keys);
    free(vals);
    return NULL;
  }
  cstmap->keys = keys;
  cstmap->vals = vals;
  cstmap->npair = npair;
  klcst_setposition(cstmap, begin, end);
  klcst_init(cstmap, &klcst_map_vfunc);
  return cstmap;
}

KlCstArray* klcst_array_create(KlCst* vals, KlFileOffset begin, KlFileOffset end) {
  KlCstArray* cstarray = klcst_alloc(KlCstArray);
  if (kl_unlikely(!cstarray)) {
      klcst_delete_raw(vals);
    return NULL;
  }
  cstarray->vals = vals;
  klcst_setposition(cstarray, begin, end);
  klcst_init(cstarray, &klcst_array_vfunc);
  return cstarray;
}

KlCstArrayGenerator* klcst_arraygenerator_create(KlStrDesc arrid, KlCst* block, KlFileOffset begin, KlFileOffset end) {
  KlCstArrayGenerator* cstarraygenerator = klcst_alloc(KlCstArrayGenerator);
  if (kl_unlikely(!cstarraygenerator)) {
      klcst_delete_raw(block);
    return NULL;
  }
  cstarraygenerator->arrid = arrid;
  cstarraygenerator->block = block;
  klcst_setposition(cstarraygenerator, begin, end);
  klcst_init(cstarraygenerator, &klcst_arraygenerator_vfunc);
  return cstarraygenerator;
}

KlCstClass* klcst_class_create(KlCstClassFieldDesc* fields, KlCst** vals, size_t nfield, KlCst* base, KlFileOffset begin, KlFileOffset end) {
  KlCstClass* cstclass = klcst_alloc(KlCstClass);
  if (kl_unlikely(!cstclass)) {
    for (size_t i = 0; i < nfield; ++i) {
      klcst_delete_raw(vals[i]);
    }
    free(fields);
    free(vals);
    return NULL;
  }
  cstclass->fields = fields;
  cstclass->vals = vals;
  cstclass->nfield = nfield;
  cstclass->baseclass = base;
  klcst_setposition(cstclass, begin, end);
  klcst_init(cstclass, &klcst_class_vfunc);
  return cstclass;
}

KlCstConstant* klcst_constant_create_string(KlStrDesc string, KlFileOffset begin, KlFileOffset end) {
  KlCstConstant* cstconstant = klcst_alloc(KlCstConstant);
  if (kl_unlikely(!cstconstant)) return NULL;
  cstconstant->con.string = string;
  cstconstant->con.type = KLC_STRING;
  klcst_setposition(cstconstant, begin, end);
  klcst_init(cstconstant, &klcst_constant_vfunc);
  return cstconstant;
}

KlCstConstant* klcst_constant_create_integer(KlCInt intval, KlFileOffset begin, KlFileOffset end) {
  KlCstConstant* cstconstant = klcst_alloc(KlCstConstant);
  if (kl_unlikely(!cstconstant)) return NULL;
  cstconstant->con.intval = intval;
  cstconstant->con.type = KLC_INT;
  klcst_setposition(cstconstant, begin, end);
  klcst_init(cstconstant, &klcst_constant_vfunc);
  return cstconstant;
}

KlCstConstant* klcst_constant_create_float(KlCFloat floatval, KlFileOffset begin, KlFileOffset end) {
  KlCstConstant* cstconstant = klcst_alloc(KlCstConstant);
  if (kl_unlikely(!cstconstant)) return NULL;
  cstconstant->con.floatval = floatval;
  cstconstant->con.type = KLC_FLOAT;
  klcst_setposition(cstconstant, begin, end);
  klcst_init(cstconstant, &klcst_constant_vfunc);
  return cstconstant;
}

KlCstConstant* klcst_constant_create_boolean(KlCBool boolval, KlFileOffset begin, KlFileOffset end) {
  KlCstConstant* cstconstant = klcst_alloc(KlCstConstant);
  if (kl_unlikely(!cstconstant)) return NULL;
  cstconstant->con.boolval = boolval;
  cstconstant->con.type = KLC_BOOL;
  klcst_setposition(cstconstant, begin, end);
  klcst_init(cstconstant, &klcst_constant_vfunc);
  return cstconstant;
}

KlCstConstant* klcst_constant_create_nil(KlFileOffset begin, KlFileOffset end) {
  KlCstConstant* cstconstant = klcst_alloc(KlCstConstant);
  if (kl_unlikely(!cstconstant)) return NULL;
  cstconstant->con.type = KLC_NIL;
  klcst_setposition(cstconstant, begin, end);
  klcst_init(cstconstant, &klcst_constant_vfunc);
  return cstconstant;
}

KlCstVararg* klcst_vararg_create(KlFileOffset begin, KlFileOffset end) {
  KlCstVararg* cstvararg = klcst_alloc(KlCstVararg);
  if (kl_unlikely(!cstvararg)) return NULL;
  klcst_setposition(cstvararg, begin, end);
  klcst_init(cstvararg, &klcst_vararg_vfunc);
  return cstvararg;
}

KlCstTuple* klcst_tuple_create(KlCst** elems, size_t nelem, KlFileOffset begin, KlFileOffset end) {
  KlCstTuple* csttuple = klcst_alloc(KlCstTuple);
  if (kl_unlikely(!csttuple)) {
    for (size_t i = 0; i < nelem; ++i) {
      klcst_delete_raw(elems[i]);
    }
    free(elems);
    return NULL;
  }
  csttuple->elems = elems;
  csttuple->nelem = nelem;
  klcst_setposition(csttuple, begin, end);
  klcst_init(csttuple, &klcst_tuple_vfunc);
  return csttuple;
}

KlCstBin* klcst_bin_create(KlTokenKind op, KlCst* loperand, KlCst* roperand, KlFileOffset begin, KlFileOffset end) {
  KlCstBin* cstbin = klcst_alloc(KlCstBin);
  if (kl_unlikely(!cstbin)) {
    klcst_delete_raw(loperand);
    klcst_delete_raw(roperand);
    return NULL;
  }
  cstbin->loperand = loperand;
  cstbin->roperand = roperand;
  cstbin->op = op;
  klcst_setposition(cstbin, begin, end);
  klcst_init(cstbin, &klcst_bin_vfunc);
  return cstbin;
}

KlCstPre* klcst_pre_create(KlTokenKind op, KlCst* operand, KlFileOffset begin, KlFileOffset end) {
  KlCstPre* cstpre = klcst_alloc(KlCstPre);
  if (kl_unlikely(!cstpre)) {
    klcst_delete_raw(operand);
    return NULL;
  }
  cstpre->op = op;
  cstpre->operand = operand;
  klcst_setposition(cstpre, begin, end);
  klcst_init(cstpre, &klcst_pre_vfunc);
  return cstpre;
}

KlCstNew* klcst_new_create(KlCst* klclass, KlCst* args, KlFileOffset begin, KlFileOffset end) {
  KlCstNew* cstnew = klcst_alloc(KlCstNew);
  if (kl_unlikely(!cstnew)) {
    klcst_delete_raw(klclass);
    klcst_delete_raw(args);
    return NULL;
  }
  cstnew->klclass = klclass;
  cstnew->args = args;
  klcst_setposition(cstnew, begin, end);
  klcst_init(cstnew, &klcst_new_vfunc);
  return cstnew;
}

KlCstYield* klcst_yield_create(KlCst* vals, KlFileOffset begin, KlFileOffset end) {
  KlCstYield* cstyield = klcst_alloc(KlCstYield);
  if (kl_unlikely(!cstyield)) {
    klcst_delete_raw(vals);
    return NULL;
  }
  cstyield->vals = vals;
  klcst_setposition(cstyield, begin, end);
  klcst_init(cstyield, &klcst_yield_vfunc);
  return cstyield;
}

KlCstPost* klcst_post_create(KlTokenKind op, KlCst* operand, KlCst* post, KlFileOffset begin, KlFileOffset end) {
  KlCstPost* cstpost = klcst_alloc(KlCstPost);
  if (kl_unlikely(!cstpost)) {
    klcst_delete_raw(operand);
    klcst_delete_raw(post);
    return NULL;
  }
  cstpost->operand = operand;
  cstpost->post = post;
  cstpost->op = op;
  klcst_setposition(cstpost, begin, end);
  klcst_init(cstpost, &klcst_post_vfunc);
  return cstpost;
}

KlCstCall* klcst_call_create(KlCst* callable, KlCst* args, KlFileOffset begin, KlFileOffset end) {
  KlCstCall* cstcall = klcst_alloc(KlCstCall);
  if (kl_unlikely(!cstcall)) {
    klcst_delete_raw(callable);
    klcst_delete_raw(args);
    return NULL;
  }
  cstcall->callable = callable;
  cstcall->args = args;
  klcst_setposition(cstcall, begin, end);
  klcst_init(cstcall, &klcst_call_vfunc);
  return cstcall;
}

KlCstFunc* klcst_func_create(KlCst* block, KlCst* params, bool vararg, bool is_method, KlFileOffset begin, KlFileOffset end) {
  KlCstFunc* cstfunc = klcst_alloc(KlCstFunc);
  if (kl_unlikely(!cstfunc)) {
    klcst_delete_raw(block);
    free(params);
    return NULL;
  }
  cstfunc->block = block;
  cstfunc->params = params;
  cstfunc->vararg = vararg;
  cstfunc->is_method = is_method;
  klcst_setposition(cstfunc, begin, end);
  klcst_init(cstfunc, &klcst_func_vfunc);
  return cstfunc;
}

KlCstDot* klcst_dot_create(KlCst* operand, KlStrDesc field, KlFileOffset begin, KlFileOffset end) {
  KlCstDot* cstdot = klcst_alloc(KlCstDot);
  if (kl_unlikely(!cstdot)) {
    klcst_delete_raw(operand);
    return NULL;
  }
  cstdot->operand = operand;
  cstdot->field = field;
  klcst_setposition(cstdot, begin, end);
  klcst_init(cstdot, &klcst_dot_vfunc);
  return cstdot;
}

KlCstWhere* klcst_where_create(KlCst* expr, KlCst* block, KlStrDesc tmpid, KlFileOffset begin, KlFileOffset end) {
  KlCstWhere* cstwhere = klcst_alloc(KlCstWhere);
  if (kl_unlikely(!cstwhere)) {
    klcst_delete_raw(expr);
    klcst_delete_raw(block);
    return NULL;
  }
  cstwhere->expr = expr;
  cstwhere->block = block;
  cstwhere->tmpid = tmpid;
  klcst_setposition(cstwhere, begin, end);
  klcst_init(cstwhere, &klcst_where_vfunc);
  return cstwhere;
}



static void klcst_id_destroy(KlCstIdentifier* cstid) {
  (void)cstid;
}

static void klcst_map_destroy(KlCstMap* cstmap) {
  KlCst** keys = cstmap->keys;
  KlCst** vals = cstmap->vals;
  size_t npair = cstmap->npair;
  for (size_t i = 0; i < npair; ++i) {
    klcst_delete_raw(keys[i]);
    klcst_delete_raw(vals[i]);
  }
  free(keys);
  free(vals);
}

static void klcst_array_destroy(KlCstArray* cstarray) {
  klcst_delete_raw(cstarray->vals);
}

static void klcst_arraygenerator_destroy(KlCstArrayGenerator* cstarraygenerator) {
  klcst_delete_raw(cstarraygenerator->block);
}

static void klcst_class_destroy(KlCstClass* cstclass) {
  KlCst** vals = cstclass->vals;
  size_t nfield = cstclass->nfield;
  for (size_t i = 0; i < nfield; ++i) {
    if (vals[i]) klcst_delete_raw(vals[i]);
  }
  if (cstclass->baseclass)
    klcst_delete_raw(cstclass->baseclass);
  free(vals);
  free(cstclass->fields);
}

static void klcst_constant_destroy(KlCstConstant* cstconstant) {
  (void)cstconstant;
}

static void klcst_vararg_destroy(KlCstVararg* cstvararg) {
  (void)cstvararg;
}

static void klcst_tuple_destroy(KlCstTuple* csttuple) {
  KlCst** elems = csttuple->elems;
  size_t nelem = csttuple->nelem;
  for (size_t i = 0; i < nelem; ++i) {
    klcst_delete_raw(elems[i]);
  }
  free(elems);
}

static void klcst_bin_destroy(KlCstBin* cstbin) {
  klcst_delete_raw(cstbin->loperand);
  klcst_delete_raw(cstbin->roperand);
}

static void klcst_pre_destroy(KlCstPre* cstpre) {
  klcst_delete_raw(cstpre->operand);
}

static void klcst_new_destroy(KlCstNew* cstnew) {
  klcst_delete_raw(cstnew->args);
  klcst_delete_raw(cstnew->klclass);
}

static void klcst_yield_destroy(KlCstYield* cstyield) {
  klcst_delete_raw(cstyield->vals);
}

static void klcst_post_destroy(KlCstPost* cstpost) {
  klcst_delete_raw(cstpost->operand);
  klcst_delete_raw(cstpost->post);
}

static void klcst_call_destroy(KlCstCall* cstcall) {
  klcst_delete_raw(cstcall->callable);
  klcst_delete_raw(cstcall->args);
}

static void klcst_dot_destroy(KlCstDot* cstdot) {
  klcst_delete_raw(cstdot->operand);
}

static void klcst_func_destroy(KlCstFunc* cstfunc) {
  klcst_delete_raw(cstfunc->params);
  klcst_delete_raw(cstfunc->block);
}

static void klcst_where_destroy(KlCstWhere* cstwhere) {
  klcst_delete_raw(cstwhere->expr);
  klcst_delete_raw(cstwhere->block);
}
