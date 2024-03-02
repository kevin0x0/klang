#ifndef KEVCC_KLANG_INCLUDE_VM_KLTHROW_H
#define KEVCC_KLANG_INCLUDE_VM_KLTHROW_H

#include "klang/include/vm/klexception.h"
#include "klang/include/value/klvalue.h"
#include <stdarg.h>


typedef struct tagKlThrowInfo {
  KlException type;   /* exception type */
  size_t buflen;
  struct {
    KlValue user;     /* exception defined by user, only valid when exception == KL_E_USER. */
    char* message;    /* builtin exception description message */
  } exception;
} KlThrowInfo;

bool klthrow_init(KlThrowInfo* info, KlMM* klmm, size_t buflen);
static inline void klthrow_destroy(KlThrowInfo* info, KlMM* klmm);
static inline KlGCObject* klthrow_propagate(KlThrowInfo* info, KlGCObject* gclist);

KlException klthrow_internal(KlThrowInfo* info, KlException type, const char* format, va_list arglist);
KlException klthrow_user(KlThrowInfo* info, KlValue* user_exception);

static inline KlGCObject* klthrow_propagate(KlThrowInfo* info, KlGCObject* gclist) {
  if (info->type == KL_E_USER && klvalue_collectable(&info->exception.user))
    klmm_gcobj_mark_accessible(klvalue_getgcobj(&info->exception.user), gclist);
  return gclist;
}

static inline void klthrow_destroy(KlThrowInfo* info, KlMM* klmm) {
  klmm_free(klmm, info->exception.message, info->buflen * sizeof (char));
}


#endif

