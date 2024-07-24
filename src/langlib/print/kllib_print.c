#include "include/klapi.h"
#include "include/lang/klconfig.h"
#include "include/misc/klutils.h"

static KlException kllib_print(KlState* state);
static KlException kllib_create_print(KlState* state, const char* globalname);

KlException KLCONFIG_LIBRARY_PRINT_ENTRYFUNCNAME(KlState* state) {
  KLAPI_PROTECT(klapi_checkstack(state, 2));
  KLAPI_PROTECT(klapi_pushstring(state, "ostream"));
  klapi_loadglobal(state);

  KLAPI_PROTECT(klapi_pushstring(state, "stdout"));
  klapi_class_getfield(state, klapi_getobj(state, -2, KlClass*), klapi_getstring(state, -1), klapi_pointer(state, -1));
  KLAPI_PROTECT(kllib_create_print(state, "print"));

  KLAPI_PROTECT(klapi_pushstring(state, "stderr"));
  klapi_class_getfield(state, klapi_getobj(state, -2, KlClass*), klapi_getstring(state, -1), klapi_pointer(state, -1));
  KLAPI_PROTECT(kllib_create_print(state, "debug"));
  return klapi_return(state, 0);
}

static KlException kllib_print(KlState* state) {
  KLAPI_PROTECT(klapi_checkstack(state, 3));
  KlValue* ostream = klapi_getref(state, 0);
  KlString* methodname = klvalue_getobj(klapi_getref(state, 1), KlString*);
  KlValue* delim = klapi_getref(state, 2);
  KlValue* end = klapi_getref(state, 3);
  size_t narg = klapi_narg(state);
  for (size_t i = 0; i < narg; ++i) {
    klapi_pushvalue(state, ostream);
    klapi_pushvalue(state, klapi_accessb(state, i));
    klapi_pushvalue(state, i == narg - 1 ? end : delim);
    KlValue write;
    if (kl_unlikely(!klapi_getmethod(state, ostream, methodname, &write)))
      return klapi_throw_internal(state, KL_E_INVLD, "output stream is broken");
    KLAPI_PROTECT(klapi_scall(state, &write, 3, 0));
  }
  return klapi_return(state, 0);
}

static KlException kllib_create_print(KlState* state, const char* globalname) {
  KLAPI_PROTECT(klapi_checkstack(state, 4));
  klapi_pushvalue(state,klapi_access(state, -1));
  KLAPI_PROTECT(klapi_pushstring(state, "write"));
  KLAPI_PROTECT(klapi_pushstring(state, "\t"));
  KLAPI_PROTECT(klapi_pushstring(state, "\n"));
  KLAPI_PROTECT(klapi_mkcclosure(state, -5, kllib_print, 4));
  klapi_popclose(state, 4);
  KLAPI_PROTECT(klapi_pushstring(state, globalname));
  KLAPI_PROTECT(klapi_storeglobal(state, klapi_getstring(state, -1), -2));
  klapi_pop(state, 2);
  return KL_E_NONE;
}
