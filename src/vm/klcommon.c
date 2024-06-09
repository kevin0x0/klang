#include "include/vm/klcommon.h"
#include "include/value/klbuiltinclass.h"
#include "include/value/klvalue.h"
#include "include/vm/klexception.h"
#include "include/misc/klutils.h"

static KlClass* klcommon_phonyclass(KlMM* klmm);
static KlException klcommon_null_contructor(KlClass* klclass, KlMM* klmm, KlValue* value);

static KlException klcommon_null_contructor(KlClass* klclass, KlMM* klmm, KlValue* value) {
  kl_unused(klclass);
  kl_unused(klmm);
  kl_unused(value);
  return KL_E_INVLD;
}
static KlClass* klcommon_phonyclass(KlMM* klmm) {
  KlClass* klclass = klclass_create(klmm, 5, KLOBJECT_DEFAULT_ATTROFF, NULL, klcommon_null_contructor);
  if (kl_unlikely(!klclass)) return NULL;
  klclass_final(klclass);
  return klclass;
}


KlCommon* klcommon_create(KlMM* klmm, KlStrPool* strpool, KlMapNodePool* mapnodepool) {
  KlCommon* common = (KlCommon*)klmm_alloc(klmm, sizeof (KlCommon));
  if (kl_unlikely(!common)) return NULL;
  common->ref_count = 0;

  bool done = true;
  done = done && (common->string.len = klstrpool_new_string(strpool, "$"));
  done = done && (common->string.neg = klstrpool_new_string(strpool, "u-"));
  done = done && (common->string.add = klstrpool_new_string(strpool, "+"));
  done = done && (common->string.sub = klstrpool_new_string(strpool, "-"));
  done = done && (common->string.mul = klstrpool_new_string(strpool, "*"));
  done = done && (common->string.div = klstrpool_new_string(strpool, "/"));
  done = done && (common->string.idiv = klstrpool_new_string(strpool, "//"));
  done = done && (common->string.mod = klstrpool_new_string(strpool, "%"));
  done = done && (common->string.call = klstrpool_new_string(strpool, "()"));
  done = done && (common->string.concat = klstrpool_new_string(strpool, ".."));
  done = done && (common->string.index = klstrpool_new_string(strpool, "=[]"));
  done = done && (common->string.indexas = klstrpool_new_string(strpool, "[]="));
  done = done && (common->string.eq = klstrpool_new_string(strpool, "=="));
  done = done && (common->string.neq = klstrpool_new_string(strpool, "!="));
  done = done && (common->string.lt = klstrpool_new_string(strpool, "<"));
  done = done && (common->string.gt = klstrpool_new_string(strpool, ">"));
  done = done && (common->string.le = klstrpool_new_string(strpool, "<="));
  done = done && (common->string.ge = klstrpool_new_string(strpool, ">="));
  done = done && (common->string.hash = klstrpool_new_string(strpool, "[]"));
  done = done && (common->string.append = klstrpool_new_string(strpool, "<<"));
  done = done && (common->string.iter = klstrpool_new_string(strpool, "<-"));

  done = done && (common->string.iter = klstrpool_new_string(strpool, "typename"));


  KlClass* fallback = klcommon_phonyclass(klmm);
  done = done && fallback;
  for (KlType type = 0; type < KL_NTYPE; ++type)
    common->klclass.phony[type] = fallback;

  done = done && (common->klclass.map = klbuiltinclass_map(klmm, mapnodepool));
  done = done && (common->klclass.array = klbuiltinclass_array(klmm));
  done = done && (common->klclass.phony[KL_STRING] = klbuiltinclass_string(klmm, strpool));
  done = done && (common->klclass.phony[KL_INT] = klbuiltinclass_int(klmm));
  done = done && (common->klclass.phony[KL_FLOAT] = klbuiltinclass_float(klmm));
  done = done && (common->klclass.phony[KL_BOOL] = klbuiltinclass_bool(klmm));
  done = done && (common->klclass.phony[KL_NIL] = klbuiltinclass_nil(klmm));
  done = done && (common->klclass.phony[KL_KCLOSURE] = klbuiltinclass_kclosure(klmm));
  done = done && (common->klclass.phony[KL_CCLOSURE] = klbuiltinclass_cclosure(klmm));
  done = done && (common->klclass.phony[KL_COROUTINE] = klbuiltinclass_coroutine(klmm));
  if (kl_unlikely(!done)) {
    klmm_free(klmm, common, sizeof (KlCommon));
    return NULL;
  }
  return common;
}

KlGCObject* klcommon_propagate(KlCommon* common, KlGCObject* gclist) {
  klmm_gcobj_mark(klmm_to_gcobj(common->string.len), gclist);
  klmm_gcobj_mark(klmm_to_gcobj(common->string.neg), gclist);
  klmm_gcobj_mark(klmm_to_gcobj(common->string.add), gclist);
  klmm_gcobj_mark(klmm_to_gcobj(common->string.sub), gclist);
  klmm_gcobj_mark(klmm_to_gcobj(common->string.mul), gclist);
  klmm_gcobj_mark(klmm_to_gcobj(common->string.div), gclist);
  klmm_gcobj_mark(klmm_to_gcobj(common->string.idiv), gclist);
  klmm_gcobj_mark(klmm_to_gcobj(common->string.mod), gclist);
  klmm_gcobj_mark(klmm_to_gcobj(common->string.call), gclist);
  klmm_gcobj_mark(klmm_to_gcobj(common->string.concat), gclist);
  klmm_gcobj_mark(klmm_to_gcobj(common->string.index), gclist);
  klmm_gcobj_mark(klmm_to_gcobj(common->string.indexas), gclist);
  klmm_gcobj_mark(klmm_to_gcobj(common->string.eq), gclist);
  klmm_gcobj_mark(klmm_to_gcobj(common->string.neq), gclist);
  klmm_gcobj_mark(klmm_to_gcobj(common->string.lt), gclist);
  klmm_gcobj_mark(klmm_to_gcobj(common->string.gt), gclist);
  klmm_gcobj_mark(klmm_to_gcobj(common->string.le), gclist);
  klmm_gcobj_mark(klmm_to_gcobj(common->string.ge), gclist);
  klmm_gcobj_mark(klmm_to_gcobj(common->string.hash), gclist);
  klmm_gcobj_mark(klmm_to_gcobj(common->string.append), gclist);
  klmm_gcobj_mark(klmm_to_gcobj(common->string.iter), gclist);

  klmm_gcobj_mark(klmm_to_gcobj(common->klclass.map), gclist);
  klmm_gcobj_mark(klmm_to_gcobj(common->klclass.array), gclist);
  for (KlType type = 0; type < KL_NTYPE; ++type)
    klmm_gcobj_mark(klmm_to_gcobj(common->klclass.phony[type]), gclist);
  return gclist;
}
