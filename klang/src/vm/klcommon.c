#include "klang/include/vm/klcommon.h"
#include "klang/include/value/klarray.h"
#include "klang/include/value/klmap.h"
#include "klang/include/value/klvalue.h"
#include <string.h>

static KlClass* klcommon_phonyclass(KlMM* klmm);
static KlObject* klcommon_null_contructor(KlClass* klclass, KlMM* klmm);

static KlObject* klcommon_null_contructor(KlClass* klclass, KlMM* klmm) {
  (void)klclass;
  (void)klmm;
  return NULL;
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
  done = done && (common->string.neg = klstrpool_new_string(strpool, "-u"));
  done = done && (common->string.add = klstrpool_new_string(strpool, "+"));
  done = done && (common->string.sub = klstrpool_new_string(strpool, "-"));
  done = done && (common->string.mul = klstrpool_new_string(strpool, "*"));
  done = done && (common->string.div = klstrpool_new_string(strpool, "/"));
  done = done && (common->string.mod = klstrpool_new_string(strpool, "%"));
  done = done && (common->string.call = klstrpool_new_string(strpool, "()"));
  done = done && (common->string.concat = klstrpool_new_string(strpool, ".."));
  done = done && (common->string.index = klstrpool_new_string(strpool, "[]"));
  done = done && (common->string.indexas = klstrpool_new_string(strpool, "[]="));
  done = done && (common->string.eq = klstrpool_new_string(strpool, "=="));
  done = done && (common->string.neq = klstrpool_new_string(strpool, "!="));
  done = done && (common->string.lt = klstrpool_new_string(strpool, "<"));
  done = done && (common->string.gt = klstrpool_new_string(strpool, ">"));
  done = done && (common->string.le = klstrpool_new_string(strpool, "<="));
  done = done && (common->string.ge = klstrpool_new_string(strpool, ">="));
  done = done && (common->string.hash = klstrpool_new_string(strpool, "__hash"));
  done = done && (common->string.append = klstrpool_new_string(strpool, "++"));


  KlClass* fallback = klcommon_phonyclass(klmm);
  done = done && fallback;
  for (size_t type = 0; type < KL_NTYPE; ++type)
    common->klclass.phony[type] = fallback;

  done = done && (common->klclass.map = klmap_class(klmm, mapnodepool));
  done = done && (common->klclass.array = klarray_class(klmm));
  done = done && (common->klclass.phony[KL_STRING] = klcommon_phonyclass(klmm));
  done = done && (common->klclass.phony[KL_INT] = klcommon_phonyclass(klmm));
  done = done && (common->klclass.phony[KL_BOOL] = klcommon_phonyclass(klmm));
  done = done && (common->klclass.phony[KL_NIL] = klcommon_phonyclass(klmm));
  done = done && (common->klclass.phony[KL_KCLOSURE] = klcommon_phonyclass(klmm));
  done = done && (common->klclass.phony[KL_CCLOSURE] = klcommon_phonyclass(klmm));
  done = done && (common->klclass.phony[KL_COROUTINE] = klcommon_phonyclass(klmm));
  if (kl_unlikely(!done)) {
    klmm_free(klmm, common, sizeof (KlCommon));
    return NULL;
  }
  return common;
}

KlGCObject* klcommon_propagate(KlCommon* common, KlGCObject* gclist) {
  klmm_gcobj_mark_accessible(klmm_to_gcobj(common->string.neg), gclist);
  klmm_gcobj_mark_accessible(klmm_to_gcobj(common->string.add), gclist);
  klmm_gcobj_mark_accessible(klmm_to_gcobj(common->string.sub), gclist);
  klmm_gcobj_mark_accessible(klmm_to_gcobj(common->string.mul), gclist);
  klmm_gcobj_mark_accessible(klmm_to_gcobj(common->string.div), gclist);
  klmm_gcobj_mark_accessible(klmm_to_gcobj(common->string.mod), gclist);
  klmm_gcobj_mark_accessible(klmm_to_gcobj(common->string.call), gclist);
  klmm_gcobj_mark_accessible(klmm_to_gcobj(common->string.concat), gclist);
  klmm_gcobj_mark_accessible(klmm_to_gcobj(common->string.index), gclist);
  klmm_gcobj_mark_accessible(klmm_to_gcobj(common->string.indexas), gclist);
  klmm_gcobj_mark_accessible(klmm_to_gcobj(common->string.eq), gclist);
  klmm_gcobj_mark_accessible(klmm_to_gcobj(common->string.neq), gclist);
  klmm_gcobj_mark_accessible(klmm_to_gcobj(common->string.lt), gclist);
  klmm_gcobj_mark_accessible(klmm_to_gcobj(common->string.gt), gclist);
  klmm_gcobj_mark_accessible(klmm_to_gcobj(common->string.le), gclist);
  klmm_gcobj_mark_accessible(klmm_to_gcobj(common->string.ge), gclist);
  klmm_gcobj_mark_accessible(klmm_to_gcobj(common->string.hash), gclist);
  klmm_gcobj_mark_accessible(klmm_to_gcobj(common->string.append), gclist);

  klmm_gcobj_mark_accessible(klmm_to_gcobj(common->klclass.map), gclist);
  klmm_gcobj_mark_accessible(klmm_to_gcobj(common->klclass.array), gclist);
  for (size_t type = 0; type < KL_NTYPE; ++type)
    klmm_gcobj_mark_accessible(klmm_to_gcobj(common->klclass.phony[type]), gclist);
  return gclist;
}
