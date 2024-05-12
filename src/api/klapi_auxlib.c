#include "include/klapi.h"

KlException klapi_return(KlState* state, size_t nret) {
  kl_assert(klstack_size(klstate_stack(state)) >= nret, "number of returned values exceeds the stack size, which is impossible.");
  kl_assert(nret < KLAPI_VARIABLE_RESULTS, "currently does not support this number of returned values.");
  KlCallInfo* currci = klstate_currci(state);
  KlValue* retpos = currci->base + currci->retoff;
  KlValue* respos = klapi_access(state, -nret);
  kl_assert(retpos <= respos, "number of returned values exceeds stack frame size, which is impossible");
  if (retpos == respos) return KL_E_NONE;
  size_t nwanted = currci->nret == KLAPI_VARIABLE_RESULTS ? nret : currci->nret;
  size_t ncopy = nwanted < nret ? nwanted : nret;
  while (ncopy--)
    klvalue_setvalue(respos++, retpos++);
  if (nret < nwanted)
    klexec_setnils(respos, nwanted - nret);
  if (currci->nret == KLAPI_VARIABLE_RESULTS)
    klstack_set_top(klstate_stack(state), respos + nwanted);
  return KL_E_NONE;
}
