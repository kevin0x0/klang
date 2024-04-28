#include "include/vm/klthrow.h"
#include "include/vm/klexception.h"

#include <stdio.h>
#include <string.h>

bool klthrow_init(KlThrowInfo* info, KlMM* klmm, size_t buflen) {
  char* buf = (char*)klmm_alloc(klmm, buflen * sizeof (char));
  if (kl_unlikely(!buf)) return false;
  info->type = KL_E_NONE;
  info->buflen = buflen;
  info->exception.message = buf;
  klvalue_setnil(&info->exception.eobj);
  return true;
}

KlException klthrow_internal(KlThrowInfo* info, KlException type, const char* format, va_list arglist) {
  info->type = type;
  vsnprintf(info->exception.message, info->buflen, format, arglist);
  return type;
}

KlException klthrow_link(KlThrowInfo* info, KlState* src) {
  info->type = KL_E_LINK;
  info->exception.esrc = src;
  return KL_E_LINK;
}

KlException klthrow_user(KlThrowInfo* info, KlValue* user_exception) {
  info->type = KL_E_USER;
  klvalue_setvalue(&info->exception.eobj, user_exception);
  return KL_E_USER;
}
