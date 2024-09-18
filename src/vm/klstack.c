#include "include/vm/klstack.h"
#include "include/value/klvalue.h"
#include "include/lang/kltypes.h"

#include <limits.h>

#define KLSTACK_INITSIZE      (32)
#define KLSTACK_MAXSIZE       (1024 * 1024)


/* callinfo.retoff should be able to represent any possible return position
 * and callinfo.narg should be able to represent any possible number of arguments
 * in function call, so the KLSTACK_MAXSIZE should not be too large.
 * see KlCallInfo. */
kl_static_assert(KLSTACK_MAXSIZE < KLUINT_MAX && KLSTACK_MAXSIZE < INT_MAX, "stack size limit too large");


bool klstack_init(KlStack* stack, KlMM* klmm) {
  KlValue* array = (KlValue*)klmm_alloc(klmm, KLSTACK_INITSIZE * sizeof (KlValue));
  if (!array) return false;
  for (KlValue* p = array; p != array + KLSTACK_INITSIZE; ++p)
    klvalue_setnil(p);
  stack->array = array;
  stack->curr = array;
  stack->end = array + KLSTACK_INITSIZE;
  return true;
}

KlException klstack_expand(KlStack* stack, KlMM* klmm, size_t expectedcap) {
  size_t old_capacity = klstack_capacity(stack);
  size_t old_size = klstack_size(stack);
  size_t new_capacity = old_capacity * 2 >= expectedcap ? old_capacity * 2 : expectedcap;
  if (new_capacity >= KLSTACK_MAXSIZE) {
    if (expectedcap >= KLSTACK_MAXSIZE)
      return KL_E_RANGE;
    new_capacity = expectedcap;
  }
  KlValue* array = (KlValue*)klmm_realloc(klmm, stack->array,
                                          new_capacity * sizeof (KlValue),
                                          old_capacity * sizeof (KlValue));
  if (!array) return KL_E_OOM;
  for (KlValue* p = array + old_capacity; p != array + new_capacity; ++p)
    klvalue_setnil(p);
  stack->curr = array + old_size;
  stack->array = array;
  stack->end = array + new_capacity;
  return KL_E_NONE;
}
