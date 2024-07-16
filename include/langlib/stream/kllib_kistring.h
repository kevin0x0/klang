#ifndef _KLANG_INCLUDE_LANGLIB_STREAM_KLLIB_KISTRING_H_
#define _KLANG_INCLUDE_LANGLIB_STREAM_KLLIB_KISTRING_H_
#include "deps/k/include/kio/ki.h"
#include "include/mm/klmm.h"
#include "include/value/klstring.h"

typedef struct tagKiString KiString;

Ki* kistring_create(KlString* str);
KlGCObject* kistring_prop(const KiString* kistring, KlMM* klmm, KlGCObject* gclist);

#endif
