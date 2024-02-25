#include "klang/include/vm/klstack.h"
#include "klang/include/value/klvalue.h"

#define KLSTACK_INITSIZE      (32)


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

bool klstack_expand(KlStack* stack, KlMM* klmm) {
  size_t old_capacity = klstack_capacity(stack);
  size_t new_capacity = old_capacity * 2;
  KlValue* array = (KlValue*)klmm_realloc(klmm, stack->array,
                                          new_capacity * sizeof (KlValue),
                                          old_capacity * sizeof (KlValue));
  if (!array) return false;
  for (KlValue* p = array + old_capacity; p != array + new_capacity; ++p)
    klvalue_setnil(p);
  stack->curr += array - stack->array;
  stack->array = array;
  stack->end = array + new_capacity;
  return true;
}
