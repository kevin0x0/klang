#include "include/value/klbuiltinclass.h"
#include "include/value/klarray.h"
#include "include/value/klstate.h"


/* array */
static KlException array_constructor(KlClass* klclass, KlMM* klmm, KlValue* value) {
  kl_unused(klclass);
  KlArray* arr = klarray_create(klmm, 1);
  if (kl_unlikely(!arr)) return KL_E_OOM;
  klvalue_setobj(value, arr, KL_ARRAY);
  return KL_E_NONE;
}

KlClass* klbuiltinclass_array(KlMM* klmm) {
  KlClass* klclass = klclass_create(klmm, 2, KLOBJECT_DEFAULT_ATTROFF, NULL, array_constructor);
  if (kl_unlikely(!klclass)) return NULL;
  klclass_final(klclass);
  return klclass;
}

/* map */
static KlException map_constructor(KlClass* klclass, KlMM* klmm, KlValue* value) {
  kl_unused(klclass);
  kl_unused(klmm);
  KlMap* map = klmap_create(klmm, 3);
  if (kl_unlikely(!map)) return KL_E_OOM;
  klvalue_setobj(value, map, KL_MAP);
  return KL_E_NONE;
}

KlClass* klbuiltinclass_map(KlMM* klmm) {
  KlClass* klclass = klclass_create(klmm, 2, KLOBJECT_DEFAULT_ATTROFF, NULL, map_constructor);
  if (kl_unlikely(!klclass)) return NULL;
  klclass_final(klclass);
  return klclass;
}

static KlException string_constructor(KlClass* klclass, KlMM* klmm, KlValue* value) {
  kl_unused(klmm);
  KlString* str = klstrpool_new_string(klcast(KlStrPool*, klclass_constructor_data(klclass)), "hello");
  if (kl_unlikely(!str)) return KL_E_OOM;
  klvalue_setobj(value, str, klvalue_getstringtype(str));
  return KL_E_NONE;
}

KlClass* klbuiltinclass_string(KlMM* klmm, KlStrPool* strpool) {
  KlClass* klclass = klclass_create(klmm, 2, KLOBJECT_DEFAULT_ATTROFF, strpool, string_constructor);
  if (kl_unlikely(!klclass)) return NULL;
  klclass_final(klclass);
  return klclass;
}

static KlException defaultcfunc(KlState* state) {
  return klstate_throw(state, KL_E_INVLD, "you can not call this function");
}

static KlException cfunc_constructor(KlClass* klclass, KlMM* klmm, KlValue* value) {
  kl_unused(klclass);
  kl_unused(klmm);
  klvalue_setcfunc(value, defaultcfunc);
  return KL_E_NONE;
}

KlClass* klbuiltinclass_cfunc(KlMM* klmm) {
  KlClass* klclass = klclass_create(klmm, 2, KLOBJECT_DEFAULT_ATTROFF, NULL, cfunc_constructor);
  if (kl_unlikely(!klclass)) return NULL;
  klclass_final(klclass);
  return klclass;
}

static KlException nil_constructor(KlClass* klclass, KlMM* klmm, KlValue* value) {
  kl_unused(klclass);
  kl_unused(klmm);
  klvalue_setnil(value);
  return KL_E_NONE;
}

KlClass* klbuiltinclass_kclosure(KlMM* klmm) {
  KlClass* klclass = klclass_create(klmm, 2, KLOBJECT_DEFAULT_ATTROFF, NULL, nil_constructor);
  if (kl_unlikely(!klclass)) return NULL;
  klclass_final(klclass);
  return klclass;
}

KlClass* klbuiltinclass_cclosure(KlMM* klmm) {
  KlClass* klclass = klclass_create(klmm, 2, KLOBJECT_DEFAULT_ATTROFF, NULL, nil_constructor);
  if (kl_unlikely(!klclass)) return NULL;
  klclass_final(klclass);
  return klclass;
}

KlClass* klbuiltinclass_coroutine(KlMM* klmm) {
  KlClass* klclass = klclass_create(klmm, 2, KLOBJECT_DEFAULT_ATTROFF, NULL, nil_constructor);
  if (kl_unlikely(!klclass)) return NULL;
  klclass_final(klclass);
  return klclass;
}

KlClass* klbuiltinclass_kfunc(KlMM* klmm) {
  KlClass* klclass = klclass_create(klmm, 2, KLOBJECT_DEFAULT_ATTROFF, NULL, nil_constructor);
  if (kl_unlikely(!klclass)) return NULL;
  klclass_final(klclass);
  return klclass;
}

KlClass* klbuiltinclass_state(KlMM* klmm) {
  KlClass* klclass = klclass_create(klmm, 2, KLOBJECT_DEFAULT_ATTROFF, NULL, nil_constructor);
  if (kl_unlikely(!klclass)) return NULL;
  klclass_final(klclass);
  return klclass;
}

static KlException int_constructor(KlClass* klclass, KlMM* klmm, KlValue* value) {
  kl_unused(klclass);
  kl_unused(klmm);
  klvalue_setint(value, 0);
  return KL_E_NONE;
}

KlClass* klbuiltinclass_int(KlMM* klmm) {
  KlClass* klclass = klclass_create(klmm, 2, KLOBJECT_DEFAULT_ATTROFF, NULL, int_constructor);
  if (kl_unlikely(!klclass)) return NULL;
  klclass_final(klclass);
  return klclass;
}

static KlException float_constructor(KlClass* klclass, KlMM* klmm, KlValue* value) {
  kl_unused(klclass);
  kl_unused(klmm);
  klvalue_setfloat(value, 0.0);
  return KL_E_NONE;
}

KlClass* klbuiltinclass_float(KlMM* klmm) {
  KlClass* klclass = klclass_create(klmm, 2, KLOBJECT_DEFAULT_ATTROFF, NULL, float_constructor);
  if (kl_unlikely(!klclass)) return NULL;
  klclass_final(klclass);
  return klclass;
}

static KlException bool_constructor(KlClass* klclass, KlMM* klmm, KlValue* value) {
  kl_unused(klclass);
  kl_unused(klmm);
  klvalue_setbool(value, KL_TRUE);
  return KL_E_NONE;
}

KlClass* klbuiltinclass_bool(KlMM* klmm) {
  KlClass* klclass = klclass_create(klmm, 2, KLOBJECT_DEFAULT_ATTROFF, NULL, bool_constructor);
  if (kl_unlikely(!klclass)) return NULL;
  klclass_final(klclass);
  return klclass;
}

KlClass* klbuiltinclass_nil(KlMM* klmm) {
  KlClass* klclass = klclass_create(klmm, 0, KLOBJECT_DEFAULT_ATTROFF, NULL, nil_constructor);
  if (kl_unlikely(!klclass)) return NULL;
  klclass_final(klclass);
  return klclass;
}
