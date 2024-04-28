#include "include/value/klvalue.h"

static const char* klvalue_typenames[KL_NTYPE] = {
  [KL_FLOAT] = "float",
  [KL_INT] = "integer",
  [KL_NIL] = "nil",
  [KL_BOOL] = "bool",
  [KL_CFUNCTION] = "C function",
  [KL_MAP] = "map",
  [KL_ARRAY] = "array",
  [KL_STRING] = "string",
  [KL_KCLOSURE] = "closure",
  [KL_CCLOSURE] = "C closure",
  [KL_COROUTINE] = "coroutine",
  [KL_CLASS] = "class",
  [KL_OBJECT] = "object",
};

const char* klvalue_typename(KlType type) {
  return klvalue_typenames[type];
}
