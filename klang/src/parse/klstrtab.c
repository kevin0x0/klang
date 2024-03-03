#include "klang/include/parse/klstrtab.h"
#include <stdlib.h>

KlStrTab* klstrtab_create(void) {
  KlStrTab* strtab = (KlStrTab*)malloc(sizeof (KlStrTab));
  if (kl_unlikely(!strtab)) return NULL;
  strtab->stack = NULL;
  strtab->curr = NULL;
  strtab->end = NULL;
  if (kl_unlikely(!klstrtab_grow(strtab, KLSTRTAB_EXTRA))) {
    free(strtab);
    return NULL;
  }
  return strtab;
}

void klstrtab_delete(KlStrTab* strtab) {
  kl_assert(strtab != NULL && strtab->stack != NULL, "klstrtab_delete(): invalid strtab");

  free(strtab->stack);
  free(strtab);
}

char* klstrtab_grow(KlStrTab* strtab, size_t extra) {
  size_t expectedcap = klstrtab_size(strtab) + extra;
  size_t newcap = 2 * klstrtab_capacity(strtab);
  if (newcap < expectedcap) newcap = expectedcap;
  char* newstk = (char*)realloc(strtab->stack, newcap);
  if (kl_unlikely(!newstk)) return NULL;
  size_t curroffset = klstrtab_size(strtab);
  strtab->stack = newstk;
  strtab->curr = newstk + curroffset;
  strtab->end = newstk + newcap;
  return strtab->curr;
}
