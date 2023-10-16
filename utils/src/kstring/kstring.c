#include "utils/include/kstring/kstring.h"

#include <stdlib.h>


KString* kstring_create_len(size_t len) {
  KString* kstr = (KString*)malloc(sizeof (KString) - sizeof (kstr->string) + len * sizeof (char));
  if (kstr) kstr->len = len;
  return kstr;
}

KString* kstring_create(const char* cstr) {
  KString* kstr = kstring_create_len(strlen(cstr));
  if (!kstr) return NULL;
    memcpy(kstr->string, cstr, kstr->len);
  return kstr;
}

KString* kstring_create_copy(const KString* kstr) {
  size_t size = sizeof (KString) - sizeof (kstr->string) + kstr->len * sizeof (char);
  KString* cpy = (KString*)malloc(size);
  if (!cpy) return NULL;
  memcpy(cpy, kstr, size);
  return cpy;
}

KString* kstring_create_buf(const char* cstr_begin, const char* cstr_end) {
  KString* kstr = kstring_create_len(cstr_end - cstr_begin);
  if (!kstr) return NULL;
  memcpy(kstr->string, cstr_begin, cstr_end - cstr_begin);
  return kstr;
}

void kstring_delete(KString* kstr) {
  free(kstr);
}


KString* kstring_concat(const KString* kstr1, const KString* kstr2) {
  KString* kstr = kstring_create_len(kstr1->len + kstr2->len);
  if (!kstr) return NULL;
  memcpy(kstr->string, kstr1->string, sizeof (char) * kstr1->len);
  memcpy(kstr->string + kstr1->len, kstr2->string, sizeof (char) * kstr2->len);
  return kstr;
}
