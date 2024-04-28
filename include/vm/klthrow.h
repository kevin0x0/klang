#ifndef KEVCC_KLANG_INCLUDE_VM_KLTHROW_H
#define KEVCC_KLANG_INCLUDE_VM_KLTHROW_H

#include "include/vm/klexception.h"
#include "include/value/klvalue.h"
#include <stdarg.h>

typedef struct tagKlState KlState;

typedef struct tagKlThrowInfo {
  KlException type;   /* exception type */
  size_t buflen;
  struct {
    union {
      KlValue eobj;   /* exception defined by user, only valid when exception == KL_E_USER. */
      KlState* esrc;  /* the vm where exception occurrs */
    };
    char* message;    /* builtin exception description message */
  } exception;
} KlThrowInfo;

bool klthrow_init(KlThrowInfo* info, KlMM* klmm, size_t buflen);
static inline void klthrow_destroy(KlThrowInfo* info, KlMM* klmm);
static inline KlGCObject* klthrow_propagate(KlThrowInfo* info, KlGCObject* gclist);

KlException klthrow_internal(KlThrowInfo* info, KlException type, const char* format, va_list arglist);
KlException klthrow_link(KlThrowInfo* info, KlState* src);
KlException klthrow_user(KlThrowInfo* info, KlValue* user_exception);

static inline KlGCObject* klthrow_propagate(KlThrowInfo* info, KlGCObject* gclist) {
  if (info->type == KL_E_USER && klvalue_collectable(&info->exception.eobj))
    klmm_gcobj_mark_accessible(klvalue_getgcobj(&info->exception.eobj), gclist);
  if (info->type == KL_E_LINK)
    klmm_gcobj_mark_accessible(klmm_to_gcobj(info->exception.esrc), gclist);
  return gclist;
}

static inline void klthrow_destroy(KlThrowInfo* info, KlMM* klmm) {
  klmm_free(klmm, info->exception.message, info->buflen * sizeof (char));
}


#endif

