#include "include/klapi.h"
#include "include/misc/klutils.h"
#include "include/value/klstate.h"
#include "deps/k/include/lib/lib.h"

KlException klapi_trycall(KlState* state, KlValue* callable, size_t narg, size_t nret, KlValue* respos) {
  ptrdiff_t errhandler_save = klexec_savestack(state, klstate_stktop(state) - narg - 1);
  ptrdiff_t respos_save = klexec_savestack(state, respos);
  kl_assert(errhandler_save >= 0, "errhandler not provided");
  KlCallInfo* currci = klstate_currci(state);
  KlException exception = klapi_call(state, callable, narg, nret, respos);
  /* if no exception occurred, just return */
  if (kl_likely(!exception)) return exception;
  /* else handle exception */
  exception = klapi_call(state, klexec_restorestack(state, errhandler_save), 0, nret, klexec_restorestack(state, respos_save));
  if (kl_unlikely(exception)) return exception;
  klstate_setcurrci(state, currci);
  return KL_E_NONE;
}

KlException klapi_loadlib(KlState* state, const char* libpath, const char* entryfunction) {
  /* open library */
  KLib handle = klib_dlopen(libpath);
  if (kl_unlikely(klib_failed(handle))) 
    return klstate_throw(state, KL_E_INVLD, "can not open library: %s", libpath);

  /* find entry point */
  entryfunction = entryfunction ? entryfunction : "kllib_init";
  KlCFunction* init = (KlCFunction*)klib_dlsym(handle, entryfunction);
  if (kl_unlikely(!init)) {
    klib_dlclose(handle);
    return klstate_throw(state, KL_E_INVLD, "can not find entry point: %s", entryfunction);
  }

  /* call the entry function */
  KlException exception = klapi_pushcfunc(state, init);
  if (kl_unlikely(exception)) return exception;
  return klapi_call(state, klapi_access(state, -1), 0, KLAPI_VARIABLE_RESULTS, klapi_access(state, -1));
}
