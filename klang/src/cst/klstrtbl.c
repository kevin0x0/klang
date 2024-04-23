#include "klang/include/cst/klstrtbl.h"
#include <stdlib.h>
#include <string.h>

KlStrTbl* klstrtbl_create(void) {
  KlStrTbl* strtbl = (KlStrTbl*)malloc(sizeof (KlStrTbl));
  if (kl_unlikely(!strtbl)) return NULL;
  strtbl->stack = NULL;
  strtbl->curr = NULL;
  strtbl->end = NULL;
  if (kl_unlikely(!klstrtbl_grow(strtbl, KLSTRTAB_EXTRA))) {
    free(strtbl);
    return NULL;
  }
  return strtbl;
}

void klstrtbl_delete(KlStrTbl* strtbl) {
  kl_assert(strtbl != NULL && strtbl->stack != NULL, "klstrtbl_delete(): invalid strtbl");

  free(strtbl->stack);
  free(strtbl);
}

char* klstrtbl_grow(KlStrTbl* strtbl, size_t extra) {
  size_t expectedcap = klstrtbl_size(strtbl) + extra;
  size_t newcap = 2 * klstrtbl_capacity(strtbl);
  if (newcap < expectedcap) newcap = expectedcap;
  char* newstk = (char*)realloc(strtbl->stack, sizeof (char) * newcap);
  if (kl_unlikely(!newstk)) return NULL;
  size_t curroffset = klstrtbl_size(strtbl);
  strtbl->stack = newstk;
  strtbl->curr = newstk + curroffset;
  strtbl->end = newstk + newcap;
  return strtbl->curr;
}

char* klstrtbl_concat(KlStrTbl* strtbl, KlStrDesc left, KlStrDesc right) {
  if (left.id + left.length == right.id) {
    return klstrtbl_getstring(strtbl, left.id);
  } else if (klstrtbl_getstring(strtbl, left.id) + left.length == strtbl->curr) {
    char* res = klstrtbl_allocstring(strtbl, right.length);
    if (kl_unlikely(!res)) return NULL;
    strncpy(res, klstrtbl_getstring(strtbl, right.id), right.length);
    return klstrtbl_getstring(strtbl, left.id);
  } else {
    char* res = klstrtbl_allocstring(strtbl, left.length + right.length);
    if (kl_unlikely(!res)) return NULL;
    strncpy(res, klstrtbl_getstring(strtbl, left.id), left.length);
    strncpy(res + left.length, klstrtbl_getstring(strtbl, right.id), right.length);
    return res;
  }
}
