#ifndef KEVCC_KLANG_INCLUDE_VM_KLEXCEPTION_H
#define KEVCC_KLANG_INCLUDE_VM_KLEXCEPTION_H

#include <stddef.h>

typedef enum tagKlException {
  KL_E_OOM = -1, KL_E_NONE = 0, KL_E_HANDLED,
  KL_E_INVLD, KL_E_TYPE, KL_E_RANGE, KL_E_ARGNO,
  KL_E_DISMATCH, KL_E_ZERO, KL_E_LINK,
  KL_E_USER,
} KlException;


#endif
