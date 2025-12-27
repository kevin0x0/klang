#include "include/common/klconfig.h"
#include "include/klapi.h"
#include "include/mm/klmm.h"

static KlException createclass(KlState* state);

KlException KLCONFIG_LIBRARY_GC_ENTRYFUNCNAME(KlState* state) {
  KLAPI_PROTECT(klapi_checkstack(state, 1));
  KLAPI_PROTECT(klapi_pushstring(state, "gc"));
  KLAPI_PROTECT(createclass(state));
  KLAPI_PROTECT(klapi_storeglobal(state, klapi_getstring(state, -2), -1));
  return klapi_return(state, 0);
}

static KlException dogc(KlState* state) {
  klmm_do_gc(klstate_getmm(state));
  return klapi_return(state, 0);
}

static KlException memory_usage(KlState* state) {
  klapi_checkstack(state, 1);
  size_t usage = klmm_memory_usage(klstate_getmm(state));
  klapi_pushint(state, (KlInt)usage);
  return klapi_return(state, 1);
}

static KlException memory_limit(KlState* state) {
  klapi_checkstack(state, 1);
  size_t limit = klmm_memory_limit(klstate_getmm(state));
  klapi_pushint(state, (KlInt)limit);
  return klapi_return(state, 1);
}

static KlException createclass(KlState* state) {
  KLAPI_PROTECT(klapi_checkstack(state, 3));
  KlClass* castclass = klclass_create(klstate_getmm(state), 1, KLOBJECT_DEFAULT_ATTROFF, NULL, NULL);
  if (kl_unlikely(!castclass))
    return klapi_throw_internal(state, KL_E_OOM, "out of memory while creating cast");
  klapi_pushobj(state, castclass, KL_CLASS);

  KLAPI_PROTECT(klapi_pushstring(state, "dogc"));
  klapi_pushcfunc(state, dogc);
  KLAPI_PROTECT(klapi_class_newshared_normal(state, castclass, klapi_getstring(state, -2)));

  KLAPI_PROTECT(klapi_setstring(state, -2, "memory_usage"));
  klapi_setcfunc(state, -1, memory_usage);
  KLAPI_PROTECT(klapi_class_newshared_normal(state, castclass, klapi_getstring(state, -2)));

  KLAPI_PROTECT(klapi_setstring(state, -2, "memory_limit"));
  klapi_setcfunc(state, -1, memory_limit);
  KLAPI_PROTECT(klapi_class_newshared_normal(state, castclass, klapi_getstring(state, -2)));

  klapi_pop(state, 2);
  return KL_E_NONE;
}
