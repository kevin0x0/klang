#include "klang/include/klapi.h"
#include "klang/include/mm/klmm.h"
#include "klang/include/vm/klexec.h"
#include "klang/include/vm/klstate.h"
#include "klang/include/value/klref.h"

KlState* klapi_new_state(KlMM* klmm) {
  KlGCObject* original_root = klmm_get_root(klmm);
  klmm_register_root(klmm, NULL); /* disable gc */

  KlMapNodePool* mapnodepool = klmapnodepool_create(klmm);
  if (!mapnodepool) {
    klmm_register_root(klmm, original_root);
    return NULL;
  }
  klmapnodepool_pin(mapnodepool);

  KlStrPool* strpool = klstrpool_create(klmm, 32);
  if (!strpool) {
    klmm_register_root(klmm, original_root);
    klmapnodepool_unpin(mapnodepool);
    return NULL;
  }

  KlCommon* common = klcommon_create(strpool, mapnodepool);
  if (!common) {
    klmm_register_root(klmm, original_root);
    klmapnodepool_unpin(mapnodepool);
    return NULL;
  }
  klcommon_pin(common);

  KlMap* global = klmap_create(common->klclass.map, 5, mapnodepool);
  if (!global) {
    klmm_register_root(klmm, original_root);
    klmapnodepool_unpin(mapnodepool);
    klcommon_unpin(common, klmm);
    return NULL;
  }

  KlState* state = klstate_create(klmm, global, common, strpool, mapnodepool);
  if (!state) {
    klmm_register_root(klmm, original_root);
    klmapnodepool_unpin(mapnodepool);
    klcommon_unpin(common, klmm);
    return NULL;
  }
  klmm_register_root(klmm, klmm_to_gcobj(state));
  klmapnodepool_unpin(mapnodepool);
  klcommon_unpin(common, klmm);
  return state;
}

KlException klapi_loadref(KlState* state, KlClosure* clo, int stkidx, size_t refidx) {
  if (!klclosure_checkrange(clo, refidx))
    return KL_E_RANGE;
  return klapi_setvalue(state, stkidx, klref_getval(klclosure_getref(clo, refidx)));
}

KlException klapi_storeref(KlState* state, KlClosure* clo, size_t refidx, int stkidx) {
  if (!klclosure_checkrange(clo, refidx))
    return KL_E_RANGE;
  KlValue* val = klapi_access(state, stkidx);
  if (!val) return KL_E_RANGE;
  klvalue_setvalue(klref_getval(klclosure_getref(clo, refidx)), val);
  return KL_E_NONE;
}

