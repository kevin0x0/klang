#ifndef KEVCC_KLANG_INCLUDE_VALUE_KLVALUE_H
#define KEVCC_KLANG_INCLUDE_VALUE_KLVALUE_H

#include "klang/include/mm/klmm.h"
#include "klang/include/value/klbool.h"
#include "klang/include/value/klcfunc.h"

#include <stdint.h>

#define klvalue_collectable(value)          ((value)->type >= KL_COLLECTABLE)
#define klvalue_dotable(value)              ((value)->type >= KL_DOTABLE && (value)->type <= KL_DOTABLE_END)
#define klvalue_callable(value)             ((value)->type == KL_CFUNCTION || (value)->type == KL_CCLOSURE || (value)->type == KL_KCLOSURE || (value)->type == KL_COROUTINE)
#define klvalue_canrawequal(value)          ((value)->type <= KL_RAWEQUAL)
#define klvalue_checktype(value, valtype)   ((value)->type == (valtype))
#define klvalue_gettype(value)              ((value)->type)
#define klvalue_sametype(val1, val2)        (klvalue_gettype(val1) == klvalue_gettype(val2))
#define klvalue_sameinstance(val1, val2)    (klvalue_getany(val1) == klvalue_getany(val2))

#define klvalue_getobj(val, type)           ((type)((val)->value.gcobj))
#define klvalue_setobj(val, obj, type)      klvalue_setgcobj((val), (KlGCObject*)(obj), (type))

#define klvalue_equal(v1, v2)               ((v1)->value.any == (v2)->value.any && (v1)->type == (v2)->type)


typedef enum tagKlType {
  KL_NIL = 0, KL_INT, KL_BOOL, KL_CFUNCTION,  /* non-collectable type */
  KL_ID,  /* not an actual type */
  KL_COLLECTABLE , KL_STRING = KL_COLLECTABLE,
  KL_RAWEQUAL = KL_STRING,
  KL_DOTABLE, KL_MAP = KL_DOTABLE, KL_ARRAY, KL_OBJECT,
  KL_DOTABLE_END = KL_OBJECT,
  KL_CLASS, KL_KCLOSURE, KL_CCLOSURE, KL_COROUTINE,
  KL_NTYPE, /* number of types */
} KlType;

typedef int64_t KlInt;

typedef struct tagKlValue {
  union {
    KlInt intval;
    KlBool boolval;
    KlCFunction* cfunc;
    KlGCObject* gcobj;
    size_t id;
    void* any;
  } value;
  KlType type;
} KlValue;

const char* klvalue_typename(KlType type);


static inline void* klvalue_getany(KlValue* val) {
  return val->value.any;
}

static inline KlInt klvalue_getint(KlValue* val) {
  return val->value.intval;
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
  val->value.intval = 0;
  val->type = KL_NIL;
}

static inline void klvalue_setvalue(KlValue *val, KlValue *other) {
  *val = *other;
}

#endif
