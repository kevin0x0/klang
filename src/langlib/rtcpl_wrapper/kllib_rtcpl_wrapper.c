#include "include/klapi.h"
#include "include/misc/klutils.h"
#include "include/vm/klexception.h"
#include "deps/k/include/kio/kifile.h"
#include "deps/k/include/kio/kofile.h"
#include "deps/k/include/kio/kibuf.h"
#include "deps/k/include/os_spec/kfs.h"
#include "deps/k/include/string/kstring.h"

static KlException kllib_rtcpl_wrapper_compile(KlState* state);
static KlException kllib_rtcpl_wrapper_evaluate(KlState* state);
KlException kllib_init(KlState* state);


KlException kllib_init(KlState* state) {
  KLAPI_PROTECT(klapi_checkstack(state, 2));
  char* libdir = kfs_trunc_leaf(klstring_content(klapi_getstring(state, -1)));
  if (kl_unlikely(!libdir))
    return klapi_throw_internal(state, KL_E_OOM, "out of memory while init rtcplwrapper");
  char* librtcpl = kstr_concat(libdir, "runtime_compiler.so");
  free(libdir);
  if (kl_unlikely(!librtcpl))
    return klapi_throw_internal(state, KL_E_OOM, "out of memory while init rtcplwrapper");
  klapi_pushnil(state, 1);
  KLAPI_MAYFAIL(klapi_pushstring(state, librtcpl), free(librtcpl));
  free(librtcpl);
  KLAPI_PROTECT(klapi_loadlib(state, 0, NULL));

  KLAPI_PROTECT(klapi_mkcclosure(state, -5, kllib_rtcpl_wrapper_evaluate, 1));
  klapi_popclose(state, 1); /* pop and close evaluate */

  klapi_pop(state, 2);      /* bcloader and compileri is not needed */

  KLAPI_PROTECT(klapi_mkcclosure(state, -3, kllib_rtcpl_wrapper_compile, 1));
  klapi_popclose(state, 1); /* pop and close compiler */

  KLAPI_PROTECT(klapi_pushstring(state, "eval"));
  klapi_storeglobal(state, klapi_getstring(state, -1), -2);
  klapi_pop(state, 2);
  KLAPI_PROTECT(klapi_pushstring(state, "compile"));
  klapi_storeglobal(state, klapi_getstring(state, -1), -2);
  return klapi_return(state, 0);
}

static KlException kllib_rtcpl_wrapper_compile(KlState* state) {
  if (klapi_narg(state) == 0)
    return klapi_throw_internal(state, KL_E_ARGNO, "please provide source file");
  if (!klapi_checktypeb(state, 0, KL_STRING))
    return klapi_throw_internal(state, KL_E_TYPE, "please provide source file(string)");
  KLAPI_PROTECT(klapi_checkstack(state, 4));
  KlString* filepath = klapi_getstringb(state, 0);
  Ki* ki = kifile_create(klstring_content(filepath), "rb");
  if (kl_unlikely(!ki))
    return klapi_throw_internal(state, KL_E_INVLD, "can not open file: %s", klstring_content(filepath));
  Ko* ko = kofile_attach(stderr);
  if (kl_unlikely(!ko)) {
    ki_delete(ki);
    return klapi_throw_internal(state, KL_E_OOM, "out of memory while creating output stream");
  }
  size_t argbase = klapi_framesize(state);
  klapi_pushuserdata(state, ki);
  klapi_pushuserdata(state, ko);
  klapi_pushvalue(state, &klvalue_obj(filepath, KL_STRING));
  klapi_pushvalue(state, &klvalue_obj(filepath, KL_STRING));
  KLAPI_MAYFAIL(klapi_scall(state, klapi_getref(state, 0), 4, 1), ki_delete(ki); ko_delete(ko));
  ki_delete(ki);
  ko_delete(ko);
  klapi_setframesize(state, argbase + 1);
  return klapi_return(state, 1);
}

static KlException kllib_rtcpl_wrapper_evaluate(KlState* state) {
  if (klapi_narg(state) == 0 || !klapi_checktypeb(state, 0, KL_STRING))
    return klapi_throw_internal(state, KL_E_ARGNO, "please provide expression(string)");
  KLAPI_PROTECT(klapi_checkstack(state, 4));
  KlString* expr = klapi_getstringb(state, 0);
  Ki* ki = kistr_create(klstring_content(expr));
  if (kl_unlikely(!ki))
    return klapi_throw_internal(state, KL_E_OOM, "out of memory while creating input stream");
  Ko* ko = kofile_attach(stderr);
  if (kl_unlikely(!ko)) {
    ki_delete(ki);
    return klapi_throw_internal(state, KL_E_OOM, "out of memory while creating output stream");
  }
  size_t argbase = klapi_framesize(state);
  klapi_pushuserdata(state, ki);
  klapi_pushuserdata(state, ko);
  KLAPI_MAYFAIL(klapi_pushstring(state, "expression"), ki_delete(ki); ko_delete(ko));
  klapi_pushnil(state, 1);
  KLAPI_MAYFAIL(klapi_scall(state, klapi_getref(state, 0), 4, 1), ki_delete(ki); ko_delete(ko));
  ki_delete(ki);
  ko_delete(ko);
  klapi_setframesize(state, argbase + 1);
  return klapi_tailcall(state, klapi_access(state, -1), 0);
}
