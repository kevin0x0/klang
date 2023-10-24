#ifndef KEVCC_INCLUDE_KLGC_H
#define KEVCC_INCLUDE_KLGC_H
#include "klang/include/value/value.h"

KlValue* klgc_start(KlValue* root, KlValue* valuelist);
void klgc_mark_reachable(KlValue* gclist);
KlValue* klgc_clean(KlValue* valuelist, size_t* p_listlen);
void klgc_clean_all(KlValue* valuelist);

#endif
