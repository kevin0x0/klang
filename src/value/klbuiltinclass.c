#include "include/value/klbuiltinclass.h"
#include "include/value/klarray.h"
#include "include/value/klstate.h"


/* array */
static KlException klbuiltinclass_array_constructor(KlClass* arrayclass, KlMM* klmm, KlValue* value) {
  KlArray* arr = klarray_create(arrayclass, klmm, 1);
  if (kl_unlikely(!arr)) return KL_E_OOM;
  klvalue_setobj(value, arr, KL_ARRAY);
  return KL_E_NONE;
}

bool klarray_compatible(KlObject* obj) {
  return klobject_compatible(obj, klbuiltinclass_array_constructor);
}

KlClass* klbuiltinclass_array(KlMM* klmm) {
  return klclass_create(klmm, 5, klobject_attrarrayoffset(KlArray), NULL, klbuiltinclass_array_constructor);
}

/* map */
static KlException klbuiltinclass_map_constructor(KlClass* klclass, KlMM* klmm, KlValue* value) {
  kl_unused(klmm);
  KlMap* map = klmap_create(klclass, 3, klcast(KlMapNodePool*, klclass_constructor_data(klclass)));
  if (kl_unlikely(!map)) return KL_E_OOM;
  klvalue_setobj(value, map, KL_MAP);
  return KL_E_NONE;
}

KlClass* klbuiltinclass_map(KlMM* klmm, KlMapNodePool* mapnodepool) {
  return klclass_create(klmm, 5, klobject_attrarrayoffset(KlMap), mapnodepool, klbuiltinclass_map_constructor);
}

bool klmap_compatible(KlObject* obj) {
  return klobject_compatible(obj, klbuiltinclass_map_constructor);
}

static KlException klbuiltinclass_string_constructor(KlClass* klclass, KlMM* klmm, KlValue* value) {
  kl_unused(klmm);
  KlString* str = klstrpool_new_string(klcast(KlStrPool*, klclass_constructor_data(klclass)), "hello");
  if (kl_unlikely(!str)) return KL_E_OOM;
  klvalue_setobj(value, str, KL_STRING);
  return KL_E_NONE;
}

KlClass* klbuiltinclass_string(KlMM* klmm, KlStrPool* strpool) {
  KlClass* klclass = klclass_create(klmm, 5, KLOBJECT_DEFAULT_ATTROFF, strpool, klbuiltinclass_string_constructor);
  if (kl_unlikely(!klclass)) return NULL;
  klclass_final(klclass);
  return klclass;
}

static KlException klbuiltinclass_defaultcfunc(KlState* state) {
  return klstate_throw(state, KL_E_INVLD, "you can not call this function");
}

static KlException klbuiltinclass_cfunc_constructor(KlClass* klclass, KlMM* klmm, KlValue* value) {
  kl_unused(klclass);
  kl_unused(klmm);
  klvalue_setcfunc(value, klbuiltinclass_defaultcfunc);
  return KL_E_NONE;
}

KlClass* klbuiltinclass_cfunc(KlMM* klmm) {
  KlClass* klclass = klclass_create(klmm, 5, KLOBJECT_DEFAULT_ATTROFF, NULL, klbuiltinclass_cfunc_constructor);
  if (kl_unlikely(!klclass)) return NULL;
  klclass_final(klclass);
  return klclass;
}

static KlException klbuiltinclass_nil_constructor(KlClass* klclass, KlMM* klmm, KlValue* value) {
  kl_unused(klclass);
  kl_unused(klmm);
  klvalue_setnil(value);
  return KL_E_NONE;
}

KlClass* klbuiltinclass_kclosure(KlMM* klmm) {
  KlClass* klclass = klclass_create(klmm, 5, KLOBJECT_DEFAULT_ATTROFF, NULL, klbuiltinclass_nil_constructor);
  if (kl_unlikely(!klclass)) return NULL;
  klclass_final(klclass);
  return klclass;
}

KlClass* klbuiltinclass_cclosure(KlMM* klmm) {
  KlClass* klclass = klclass_create(klmm, 5, KLOBJECT_DEFAULT_ATTROFF, NULL, klbuiltinclass_nil_constructor);
  if (kl_unlikely(!klclass)) return NULL;
  klclass_final(klclass);
  return klclass;
}

KlClass* klbuiltinclass_coroutine(KlMM* klmm) {
  KlClass* klclass = klclass_create(klmm, 5, KLOBJECT_DEFAULT_ATTROFF, NULL, klbuiltinclass_nil_constructor);
  if (kl_unlikely(!klclass)) return NULL;
  klclass_final(klclass);
  return klclass;
}

KlClass* klbuiltinclass_kfunc(KlMM* klmm) {
  KlClass* klclass = klclass_create(klmm, 5, KLOBJECT_DEFAULT_ATTROFF, NULL, klbuiltinclass_nil_constructor);
  if (kl_unlikely(!klclass)) return NULL;
  klclass_final(klclass);
  return klclass;
}

KlClass* klbuiltinclass_state(KlMM* klmm) {
  KlClass* klclass = klclass_create(klmm, 5, KLOBJECT_DEFAULT_ATTROFF, NULL, klbuiltinclass_nil_constructor);
  if (kl_unlikely(!klclass)) return NULL;
  klclass_final(klclass);
  return klclass;
}

static KlException klbuiltinclass_int_constructor(KlClass* klclass, KlMM* klmm, KlValue* value) {
  kl_unused(klclass);
  kl_unused(klmm);
  klvalue_setint(value, 0);
  return KL_E_NONE;
}

KlClass* klbuiltinclass_int(KlMM* klmm) {
  KlClass* klclass = klclass_create(klmm, 5, KLOBJECT_DEFAULT_ATTROFF, NULL, klbuiltinclass_int_constructor);
  if (kl_unlikely(!klclass)) return NULL;
  klclass_final(klclass);
  return klclass;
}

static KlException klbuiltinclass_float_constructor(KlClass* klclass, KlMM* klmm, KlValue* value) {
  kl_unused(klclass);
  kl_unused(klmm);
  klvalue_setfloat(value, 0.0);
  return KL_E_NONE;
}

KlClass* klbuiltinclass_float(KlMM* klmm) {
  KlClass* klclass = klclass_create(klmm, 5, KLOBJECT_DEFAULT_ATTROFF, NULL, klbuiltinclass_float_constructor);
  if (kl_unlikely(!klclass)) return NULL;
  klclass_final(klclass);
  return klclass;
}

static KlException klbuiltinclass_bool_constructor(KlClass* klclass, KlMM* klmm, KlValue* value) {
  kl_unused(klclass);
  kl_unused(klmm);
  klvalue_setbool(value, KL_TRUE);
  return KL_E_NONE;
}

KlClass* klbuiltinclass_bool(KlMM* klmm) {
  KlClass* klclass = klclass_create(klmm, 5, KLOBJECT_DEFAULT_ATTROFF, NULL, klbuiltinclass_bool_constructor);
  if (kl_unlikely(!klclass)) return NULL;
  klclass_final(klclass);
  return klclass;
}

KlClass* klbuiltinclass_nil(KlMM* klmm) {
  KlClass* klclass = klclass_create(klmm, 5, KLOBJECT_DEFAULT_ATTROFF, NULL, klbuiltinclass_nil_constructor);
  if (kl_unlikely(!klclass)) return NULL;
  klclass_final(klclass);
  return klclass;
}
