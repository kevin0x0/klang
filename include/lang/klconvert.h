#ifndef KEVCC_KLANG_INCLUDE_LANG_KLCONVERT_H
#define KEVCC_KLANG_INCLUDE_LANG_KLCONVERT_H

#include "include/lang/kltypes.h"
#include <stddef.h>

void kllang_int2str(char* buf, size_t bufsz, KlLangInt val);
void kllang_float2str(char* buf, size_t bufsz, KlLangFloat val);
KlLangInt kllang_str2int(const char* buf, char** endptr, int base);
KlLangFloat kllang_str2float(const char* buf, char** endptr);

#endif

