#ifndef _KLANG_INCLUDE_LANGLIB_STREAM_KLLIB_STREAM_H_
#define _KLANG_INCLUDE_LANGLIB_STREAM_KLLIB_STREAM_H_

#include "include/value/klclass.h"
#include "include/value/klcfunc.h"
#include "include/value/klvalue.h"
#include "include/vm/klexception.h"
#include "deps/k/include/kio/ki.h"
#include "deps/k/include/kio/ko.h"
#include <stdbool.h>

typedef struct tagKlInputStream {
  KL_DERIVE_FROM(KlObject, __objectbase__);
  Ki* ki;
  KLOBJECT_TAIL;
} KlInputStream;

typedef struct tagKlOutputStream {
  KL_DERIVE_FROM(KlObject, __objectbase__);
  Ko* ko;
  KLOBJECT_TAIL;
} KlOutputStream;


KlException kllib_istream_createclass(KlState* state);
KlException kllib_ostream_createclass(KlState* state);
bool kllib_istream_compatible(KlValue* val);
bool kllib_ostream_compatible(KlValue* val);

void kllib_istream_set(KlInputStream* istream, Ki* ki);
void kllib_ostream_set(KlOutputStream* ostream, Ko* ko);

#endif
