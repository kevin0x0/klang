#ifndef _KLANG_INCLUDE_LANGLIB_STREAM_KLLIB_SSTRING_H_
#define _KLANG_INCLUDE_LANGLIB_STREAM_KLLIB_SSTRING_H_

#include "include/value/klclass.h"
#include "include/vm/klexception.h"

KlException kllib_istring_createclass(KlState* state, KlClass* istream);
KlException kllib_ostring_createclass(KlState* state, KlClass* ostream);

#endif
