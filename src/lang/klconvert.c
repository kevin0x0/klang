#include "include/lang/klconvert.h"
#include <stddef.h>
#include <stdio.h>

void kllang_int2str(char* buf, size_t bufsz, KlLangInt val) {
  snprintf(buf, bufsz, "%lld", val);
}

void kllang_float2str(char* buf, size_t bufsz, KlLangFloat val) {
  snprintf(buf, bufsz, "%f", val);
}
