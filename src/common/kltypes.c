#include "include/common/kltypes.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

void kltypes_int2str(char* buf, size_t bufsz, KlLangInt val) {
  snprintf(buf, bufsz, "%lld", val);
}

void kltypes_float2str(char* buf, size_t bufsz, KlLangFloat val) {
  snprintf(buf, bufsz, "%f", val);
}

KlLangInt kltypes_str2int(const char* buf, char** endptr, int base) {
  return strtoll(buf, endptr, base);
}

KlLangFloat kltypes_str2float(const char* buf, char** endptr) {
  return strtod(buf, endptr);
}
