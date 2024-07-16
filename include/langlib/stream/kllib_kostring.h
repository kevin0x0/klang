#ifndef _KLANG_INCLUDE_LANGLIB_STREAM_KLLIB_KOSTRING_H_
#define _KLANG_INCLUDE_LANGLIB_STREAM_KLLIB_KOSTRING_H_
#include "deps/k/include/kio/ko.h"
#include "include/langlib/stream/kllib_strbuf.h"
#include "include/mm/klmm.h"

typedef struct tagKoString KoString;

Ko* kostring_create(size_t size);
bool kostring_compatible(Ko* ko);
KlStringBuf* kostring_getstrbuf(KoString* ko);

#endif

