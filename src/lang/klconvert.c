#include "include/lang/klconvert.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

void kllang_int2str(char* buf, size_t bufsz, KlLangInt val) {
  snprintf(buf, bufsz, "%lld", val);
}

void kllang_float2str(char* buf, size_t bufsz, KlLangFloat val) {
  snprintf(buf, bufsz, "%f", val);
}

KlLangInt kllang_str2int(const char* buf, char** endptr, int base) {
  return strtoll(buf, endptr, base);
}

KlLangFloat kllang_str2float(const char* buf, char** endptr) {
  return strtod(buf, endptr);
}
