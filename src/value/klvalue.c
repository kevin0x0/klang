#include "include/value/klvalue.h"

static const char* const klvalue_typenames[KL_NTYPE] = {
  [KL_INT] = "integer",
  [KL_FLOAT] = "float",
  [KL_NUMBER] = "number",
  [KL_NIL] = "nil",
  [KL_BOOL] = "bool",
  [KL_CFUNCTION] = "C function",
  [KL_USERDATA] = "user data",
  [KL_STRING] = "string",
  [KL_LSTRING] = "long string",
  [KL_MAP] = "map",
  [KL_ARRAY] = "array",
  [KL_TUPLE] = "tuple",
  [KL_OBJECT] = "object",
  [KL_CLASS] = "class",
  [KL_KFUNCTION] = "K function",
  [KL_KCLOSURE] = "K closure",
  [KL_CCLOSURE] = "C closure",
  [KL_COROUTINE] = "coroutine",
};

const char* klvalue_typename(KlType type) {
  return klvalue_typenames[type];
}
