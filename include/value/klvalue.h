#ifndef KEVCC_KLANG_INCLUDE_VALUE_KLVALUE_H
#define KEVCC_KLANG_INCLUDE_VALUE_KLVALUE_H

#include "include/mm/klmm.h"
#include "include/value/klbool.h"
#include "include/value/klcfunc.h"

#include <stdbool.h>

#define klvalue_collectable(value)          ((value)->type >= KL_COLLECTABLE)
#define klvalue_dotable(value)              ((value)->type >= KL_DOTABLE && (value)->type <= KL_DOTABLE_END)
#define klvalue_callable(value)             (((value)->type >= KL_CALLABLEOBJ && (value)->type <= KL_CALLABLEOBJ_END) || (value)->type == KL_CFUNCTION)
#define klvalue_canrawequal(value)          ((value)->type <= KL_RAWEQUAL)
#define klvalue_checktype(value, valtype)   ((value)->type == (valtype))
#define klvalue_gettype(value)              ((value)->type)
#define klvalue_sametype(val1, val2)        (klvalue_gettype(val1) == klvalue_gettype(val2))

#define klvalue_getobj(val, type)           ((type)((val)->value.gcobj))
#define klvalue_setobj(val, obj, type)      klvalue_setgcobj((val), (KlGCObject*)(obj), (type))

#define klvalue_equal(v1, v2)               (klvalue_sametype((v1), (v2)) && klvalue_sameinstance((v1), (v2)))

#define klvalue_bothinteger(v1, v2)         ((v1)->type + (v2)->type == KL_INT)
#define klvalue_bothnumber(v1, v2)          ((v1)->type + (v2)->type <= KL_NUMBER)

#define KLVALUE_NIL_INIT                    { .type = KL_NIL, .value.nilval = 0 }


typedef enum tagKlType {
  KL_INT = 0, KL_FLOAT,
  KL_NUMBER, KL_ID = KL_NUMBER,   /* not actual type, KL_NUMBER is used for number(KlInt or KlFloat) fast test */
  KL_NIL, KL_BOOL, KL_CFUNCTION,  /* non-collectable type */
  KL_COLLECTABLE , KL_STRING = KL_COLLECTABLE,
  KL_RAWEQUAL = KL_STRING,
  KL_DOTABLE, KL_MAP = KL_DOTABLE, KL_ARRAY, KL_OBJECT,
  KL_DOTABLE_END = KL_OBJECT,
  KL_CLASS,
  KL_CALLABLEOBJ, KL_KCLOSURE = KL_CALLABLEOBJ, KL_CCLOSURE, KL_COROUTINE,
  KL_CALLABLEOBJ_END = KL_COROUTINE,
  KL_NTYPE, /* number of types */
} KlType;

typedef int64_t KlInt;
typedef double KlFloat;

typedef struct tagKlValue {
  union {
    KlInt nilval;
    KlInt intval;
    KlFloat floatval;
    KlBool boolval;
    KlCFunction* cfunc;
    KlGCObject* gcobj;
    size_t id;
  } value;
  KlType type;
} KlValue;

const char* klvalue_typename(KlType type);


static inline KlInt klvalue_getnil(KlValue* val) {
  return val->value.nilval;
}

static inline KlInt klvalue_getint(KlValue* val) {
  return val->value.intval;
}

static inline KlFloat klvalue_getfloat(KlValue* val) {
  return val->value.floatval;
}

static inline KlFloat klvalue_getnumber(KlValue* val) {
  return klvalue_checktype(val, KL_INT) ? klcast(KlFloat, val->value.intval) : val->value.floatval;
}

static inline KlBool klvalue_getbool(KlValue* val) {
  return val->value.boolval;
}

static inline size_t klvalue_getid(KlValue* val) {
  return val->value.id;
}

static inline KlCFunction* klvalue_getcfunc(KlValue* val) {
  return val->value.cfunc;
}

static inline KlGCObject* klvalue_getgcobj(KlValue* val) {
  return val->value.gcobj;
}

static inline void klvalue_setint(KlValue *val, KlInt intval) {
  val->value.intval = intval;
  val->type = KL_INT;
}

static inline void klvalue_setfloat(KlValue* val, KlFloat floatval) {
  val->value.floatval = floatval;
  val->type = KL_FLOAT;
}

static inline void klvalue_setbool(KlValue *val, KlBool boolval) {
  val->value.boolval = boolval;
  val->type = KL_BOOL;
}

static inline void klvalue_setid(KlValue *val, size_t id) {
  val->value.id = id;
  val->type = KL_ID;
}

static inline void klvalue_setcfunc(KlValue *val, KlCFunction* cfunc) {
  val->value.cfunc = cfunc;
  val->type = KL_CFUNCTION;
}

static inline void klvalue_setgcobj(KlValue* val, KlGCObject* gcobj, KlType type) {
  val->value.gcobj = gcobj;
  val->type = type; 
}

static inline void klvalue_setnil(KlValue *val) {
  val->value.nilval = 0;
  val->type = KL_NIL;
}

static inline void klvalue_setvalue(KlValue *val, KlValue *other) {
  *val = *other;
}


static inline bool klvalue_sameinstance(KlValue* val1, KlValue* val2) {
  kl_assert(klvalue_sametype(val1, val2), "must call this function with two values with same type");
  switch (klvalue_gettype(val1)) {
    case KL_INT: {
      return klvalue_getint(val1) == klvalue_getint(val2);
    }
    case KL_FLOAT: {
      return klvalue_getfloat(val1) == klvalue_getfloat(val2);
    }
    case KL_BOOL: {
      return klvalue_getbool(val1) == klvalue_getbool(val2);
    }
    case KL_NIL: {
      return klvalue_getnil(val1) == klvalue_getnil(val2);
    }
    case KL_CFUNCTION: {
      return klvalue_getcfunc(val1) == klvalue_getcfunc(val2);
    }
    default: {
      return klvalue_getgcobj(val1) == klvalue_getgcobj(val2);
    }
  }
}

#endif
