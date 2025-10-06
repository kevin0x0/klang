/* this file defines basic types used in klang */

#ifndef _KLANG_INCLUDE_LANG_KLTYPES_H_
#define _KLANG_INCLUDE_LANG_KLTYPES_H_

#include "include/misc/klutils.h"

#include <limits.h>
#include <stddef.h>

typedef long long KlLangInt;
typedef double KlLangFloat;
typedef KlLangInt KlLangBool;
typedef unsigned long long KlLangUInt;

kl_static_assert(sizeof (KlLangFloat) == sizeof (KlLangInt), "");
kl_static_assert(sizeof (KlLangFloat) == sizeof (KlLangBool), "");
kl_static_assert(sizeof (KlLangFloat) == sizeof (KlLangUInt), "");


typedef unsigned KlUnsigned;
typedef unsigned char KlUByte;

#define KLUINT_MAX    (UINT_MAX)


#define KLLANG_TRUE   (1)
#define KLLANG_FALSE  (0)

void kltypes_int2str(char* buf, size_t bufsz, KlLangInt val);
void kltypes_float2str(char* buf, size_t bufsz, KlLangFloat val);
KlLangInt kltypes_str2int(const char* buf, char** endptr, int base);
KlLangFloat kltypes_str2float(const char* buf, char** endptr);

#endif
