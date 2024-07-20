#ifndef _KLANG_INCLUDE_LANGLIB_STREAM_KLLIB_FILE_H_
#define _KLANG_INCLUDE_LANGLIB_STREAM_KLLIB_FILE_H_

#include "include/langlib/stream/kllib_stream.h"
#include "include/value/klclass.h"
#include "include/vm/klexception.h"

KlException kllib_ifile_createclass(KlState* state, KlClass* istream);
KlException kllib_ofile_createclass(KlState* state, KlClass* ostream);

KlException kllib_ifile_createstdin(KlState* state, KlClass* ifile);
KlException kllib_ofile_createstdout(KlState* state, KlClass* ofile);
KlException kllib_ofile_createstderr(KlState* state, KlClass* ofile);

#endif
