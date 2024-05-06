#ifndef _KLANG_INCLUDE_CODE_KLCODEVAL_H_
#define _KLANG_INCLUDE_CODE_KLCODEVAL_H_

#include "include/ast/klast.h"
#include "include/ast/klstrtbl.h"
#include <stddef.h>

typedef enum tagKlValKind {
  KLVAL_BOOL,
  KLVAL_NIL,
  KLVAL_STRING,
  KLVAL_INTEGER,  
  KLVAL_FLOAT,  
  KLVAL_JMPLIST,
  KLVAL_STACK,
  KLVAL_REF,
  KLVAL_NONE,   /* currently only used for boolean expression */
} KlValKind;

typedef unsigned KlCStkId;
typedef unsigned KlCIdx;
typedef unsigned KlCPC;

typedef struct tagKlCodeVal {
  KlValKind kind;
  union {
    KlCIdx index;
    struct {
      KlCPC tail;
      KlCPC head;
    } jmplist;
    KlCInt intval;
    KlCFloat floatval;
    KlCBool boolval;
    KlStrDesc string;
  };
} KlCodeVal;

#define klcodeval_isnumber(v)   ((v).kind == KLVAL_FLOAT || (v).kind == KLVAL_INTEGER)
#define klcodeval_isconstant(v) ((v).kind <= KLVAL_FLOAT)
#define klcodeval_isfalse(v)    (((v).kind == KLVAL_BOOL && (v).boolval == KLC_FALSE) || (v).kind == KLVAL_NIL)
#define klcodeval_istrue(v)     (klcodeval_isconstant((v)) && !klcodeval_isfalse((v)))

static inline KlCodeVal klcodeval_index(KlValKind kind, KlCIdx index);
static inline KlCodeVal klcodeval_stack(KlCStkId index);
static inline KlCodeVal klcodeval_ref(KlCIdx index);
static inline KlCodeVal klcodeval_string(KlStrDesc str);
static inline KlCodeVal klcodeval_integer(KlCInt intval);
static inline KlCodeVal klcodeval_float(KlCFloat floatval);
static inline KlCodeVal klcodeval_bool(KlCBool boolval);
static inline KlCodeVal klcodeval_nil(void);
static inline KlCodeVal klcodeval_jmplist(KlCPC pc);
static inline KlCodeVal klcodeval_none(void);

static inline KlCodeVal klcodeval_index(KlValKind kind, KlCIdx index) {
  return (KlCodeVal) { .kind = kind, .index = index };
}

static inline KlCodeVal klcodeval_stack(KlCStkId index) {
  return (KlCodeVal) { .kind = KLVAL_STACK, .index = index };
}

static inline KlCodeVal klcodeval_ref(KlCIdx index) {
  return (KlCodeVal) { .kind = KLVAL_REF, .index = index };
}

static inline KlCodeVal klcodeval_string(KlStrDesc str) {
  return (KlCodeVal) { .kind = KLVAL_STRING, .string = str };
}

static inline KlCodeVal klcodeval_integer(KlCInt intval) {
  return (KlCodeVal) { .kind = KLVAL_INTEGER, .intval = intval };
}

static inline KlCodeVal klcodeval_float(KlCFloat floatval) {
  return (KlCodeVal) { .kind = KLVAL_FLOAT, .floatval = floatval };
}

static inline KlCodeVal klcodeval_bool(KlCBool boolval) {
  return (KlCodeVal) { .kind = KLVAL_BOOL, .boolval = boolval };
}

static inline KlCodeVal klcodeval_nil(void) {
  return (KlCodeVal) { .kind = KLVAL_NIL };
}

static inline KlCodeVal klcodeval_jmplist(KlCPC pc) {
  return (KlCodeVal) { .kind = KLVAL_JMPLIST, .jmplist = { .head = pc, .tail = pc } };
}

static inline KlCodeVal klcodeval_none(void) {
  KlCodeVal val = { .kind = KLVAL_NONE };
  return val;
}

#endif

