#include "include/klapi.h"
#include "include/lang/klconfig.h"
#include "include/misc/klutils.h"
#include "include/value/klcfunc.h"
#include "include/value/klvalue.h"
#include "include/vm/klexception.h"

#include <stdlib.h>
#include <time.h>


static KlException kllib_os_clock(KlState* state);
static KlException kllib_os_getenv(KlState* state);
static KlException kllib_os_createclass(KlState* state);


KlException KLCONFIG_LIBRARY_OS_ENTRYFUNCNAME(KlState* state) {
  KLAPI_PROTECT(klapi_checkstack(state, 1));
  KLAPI_PROTECT(klapi_pushstring(state, "os"));
  KLAPI_PROTECT(kllib_os_createclass(state));
  KLAPI_PROTECT(klapi_storeglobal(state, klapi_getstring(state, -2), -1));
  return klapi_return(state, 0);
}

static KlException kllib_os_clock(KlState* state) {
  KLAPI_PROTECT(klapi_checkstack(state, 1));
  klapi_pushfloat(state, klcast(KlFloat, clock()) / klcast(KlFloat, CLOCKS_PER_SEC));
  return klapi_return(state, 1);
}

static KlException kllib_os_getenv(KlState* state) {
  if (kl_unlikely(klapi_narg(state) != 1))
    return klapi_throw_internal(state, KL_E_ARGNO, "expected exactly one argument");
  if (kl_unlikely(!klapi_checktype(state, -1, KL_STRING)))
    return klapi_throw_internal(state, KL_E_TYPE, "expected string, got %s",
                                klstring_content(klapi_typename(state, klapi_access(state, -1))));

  KlString* envkey = klapi_getstring(state, -1);
  const char* envval = getenv(klstring_content(envkey));
  KLAPI_PROTECT(klapi_checkstack(state, 1));
  if (envval) {
    KLAPI_PROTECT(klapi_pushstring(state, envval));
  } else {
    klapi_pushnil(state, 1);
  }
  return klapi_return(state, 1);
}

static KlException kllib_os_createclass(KlState* state) {
  KLAPI_PROTECT(klapi_checkstack(state, 3));
  KlClass* osclass = klclass_create(klstate_getmm(state), 1, KLOBJECT_DEFAULT_ATTROFF, NULL, NULL);
  if (kl_unlikely(!osclass))
    return klapi_throw_internal(state, KL_E_OOM, "out of memory while creating os");
  klapi_pushobj(state, osclass, KL_CLASS);

  KLAPI_PROTECT(klapi_pushstring(state, "clock"));
  klapi_pushcfunc(state, kllib_os_clock);
  KLAPI_PROTECT(klapi_class_newshared_normal(state, osclass, klapi_getstring(state, -2)));

  KLAPI_PROTECT(klapi_setstring(state, -2, "getenv"));
  klapi_setcfunc(state, -1, kllib_os_getenv);
  KLAPI_PROTECT(klapi_class_newshared_normal(state, osclass, klapi_getstring(state, -2)));

  klapi_pop(state, 2);
  return KL_E_NONE;
}
