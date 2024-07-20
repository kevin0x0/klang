#ifndef _KLANG_INCLUDE_LANGLIB_STREAM_KLLIB_STREAM_H_
#define _KLANG_INCLUDE_LANGLIB_STREAM_KLLIB_STREAM_H_

#include "include/mm/klmm.h"
#include "include/value/klclass.h"
#include "include/value/klcfunc.h"
#include "include/value/klvalue.h"
#include "include/vm/klexception.h"
#include "deps/k/include/kio/ki.h"
#include "deps/k/include/kio/ko.h"
#include <stdbool.h>


#define KLLIB_ISTREAM_FBUF    klcast(unsigned, klbit(0))
#define KLLIB_OSTREAM_FBUF    klcast(unsigned, klbit(0))


typedef KlGCObject* (*KiProp)(Ki* ki, KlMM* klmm, KlGCObject* gclist);
typedef KlGCObject* (*KoProp)(Ko* ko, KlMM* klmm, KlGCObject* gclist);

typedef struct tagKlInputStream {
  KL_DERIVE_FROM(KlObject, __objectbase__);
  Ki* ki;
  KiProp kiprop;
  unsigned option;
  KLOBJECT_TAIL;
} KlInputStream;

typedef struct tagKlOutputStream {
  KL_DERIVE_FROM(KlObject, __objectbase__);
  Ko* ko;
  KoProp koprop;
  unsigned option;
  KLOBJECT_TAIL;
} KlOutputStream;


KlException kllib_istream_createclass(KlState* state);
KlException kllib_ostream_createclass(KlState* state);
bool kllib_istream_compatible(KlValue* val);
bool kllib_ostream_compatible(KlValue* val);
Ki* kllib_istream_getki(KlInputStream* istream);
Ko* kllib_ostream_getko(KlOutputStream* ostream);

void kllib_istream_set(KlInputStream* istream, Ki* ki, KiProp kiprop);
void kllib_ostream_set(KlOutputStream* ostream, Ko* ko, KoProp koprop);

static inline void kllib_istream_setoption(KlInputStream* istream, unsigned option);
static inline void kllib_istream_clroption(KlInputStream* istream, unsigned option);
static inline bool kllib_istream_testoption(KlInputStream* istream, unsigned option);

static inline void kllib_ostream_setoption(KlOutputStream* ostream, unsigned option);
static inline void kllib_ostream_clroption(KlOutputStream* ostream, unsigned option);
static inline bool kllib_ostream_testoption(KlOutputStream* ostream, unsigned option);

static inline void kllib_istream_setoption(KlInputStream* istream, unsigned option) {
  istream->option |= option;
}

static inline void kllib_istream_clroption(KlInputStream* istream, unsigned option) {
  istream->option &= ~option;
}

static inline bool kllib_istream_testoption(KlInputStream* istream, unsigned option) {
  return istream->option & option;
}

static inline void kllib_ostream_setoption(KlOutputStream* ostream, unsigned option) {
  ostream->option |= option;
}

static inline void kllib_ostream_clroption(KlOutputStream* ostream, unsigned option) {
  ostream->option &= ~option;
}

static inline bool kllib_ostream_testoption(KlOutputStream* ostream, unsigned option) {
  return ostream->option & option;
}

#endif
