#include "include/klapi.h"
#include "include/mm/klmm.h"
#include "include/vm/klexec.h"
#include "include/value/klstate.h"
#include "include/value/klref.h"

KlState* klapi_new_state(KlMM* klmm) {
  klmm_stopgc(klmm);  /* disable gc */

  KlMapNodePool* mapnodepool = klmapnodepool_create(klmm);
  if (!mapnodepool) {
    klmm_restartgc(klmm);
    return NULL;
  }
  klmapnodepool_pin(mapnodepool);

  KlStrPool* strpool = klstrpool_create(klmm, 32);
  if (!strpool) {
    klmm_restartgc(klmm);
    klmapnodepool_unpin(mapnodepool);
    return NULL;
  }

  KlCommon* common = klcommon_create(klmm, strpool, mapnodepool);
  if (!common) {
    klmm_restartgc(klmm);
    klmapnodepool_unpin(mapnodepool);
    return NULL;
  }
  klcommon_pin(common);

  KlMap* global = klmap_create(common->klclass.map, 5, mapnodepool);
  if (!global) {
    klmm_restartgc(klmm);
    klmapnodepool_unpin(mapnodepool);
    klcommon_unpin(common, klmm);
    return NULL;
  }

  KlState* state = klstate_create(klmm, global, common, strpool, mapnodepool, NULL);
  if (!state) {
    klmm_restartgc(klmm);
    klmapnodepool_unpin(mapnodepool);
    klcommon_unpin(common, klmm);
    return NULL;
  }
  klmm_register_root(klmm, klmm_to_gcobj(state));
  klmm_restartgc(klmm);
  klmapnodepool_unpin(mapnodepool);
  klcommon_unpin(common, klmm);
  return state;
}

KlException klapi_loadglobal(KlState* state) {
  KlString* varname = klapi_getstring(state, -1);
  KlMapIter itr = klmap_searchstring(state->global, varname);
  itr ? klapi_setvalue(state, -1, &itr->value) : klapi_setnil(state, -1);
  return KL_E_NONE;
}

KlException klapi_storeglobal(KlState* state, KlString* varname) {
  KlMapIter itr = klmap_searchstring(state->global, varname);
  if (!itr) {
    if (!klmap_insertstring(state->global, varname, klapi_access(state, -1)))
      return KL_E_OOM;
  } else {
    klvalue_setvalue(&itr->value, klapi_access(state, -1));
  }
  return KL_E_NONE;
}
