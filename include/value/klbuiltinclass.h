#ifndef _KLANG_INCLUDE_VALUE_KLBUILTINCLASS_H_
#define _KLANG_INCLUDE_VALUE_KLBUILTINCLASS_H_

#include "include/value/klclass.h"
#include "include/value/klmap.h"
#include "include/value/klstring.h"


bool klarray_compatiable(KlObject* obj);
bool klmap_compatiable(KlObject* obj);

KlClass* klbuiltinclass_array(KlMM* klmm);
KlClass* klbuiltinclass_map(KlMM* klmm, KlMapNodePool* mapnodepool);
KlClass* klbuiltinclass_string(KlMM* klmm, KlStrPool* strpool);
KlClass* klbuiltinclass_cfunc(KlMM* klmm);
KlClass* klbuiltinclass_kclosure(KlMM* klmm);
KlClass* klbuiltinclass_cclosure(KlMM* klmm);
KlClass* klbuiltinclass_coroutine(KlMM* klmm);
KlClass* klbuiltinclass_kfunc(KlMM* klmm);
KlClass* klbuiltinclass_state(KlMM* klmm);
KlClass* klbuiltinclass_int(KlMM* klmm);
KlClass* klbuiltinclass_float(KlMM* klmm);
KlClass* klbuiltinclass_bool(KlMM* klmm);
KlClass* klbuiltinclass_nil(KlMM* klmm);


#endif
