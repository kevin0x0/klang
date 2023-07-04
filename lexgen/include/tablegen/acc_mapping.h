#ifndef KEVCC_LEXGEN_INCLUDE_TABLEGEN_ACC_MAPPING_H
#define KEVCC_LEXGEN_INCLUDE_TABLEGEN_ACC_MAPPING_H
#include "lexgen/include/finite_automaton/finite_automaton.h"
#include "lexgen/include/general/global_def.h"

int16_t* kev_get_acc_mapping_u16(KevFA* dfa, uint64_t* acc_mapping_array);
int8_t* kev_get_acc_mapping_u8(KevFA* dfa, uint64_t* acc_mapping_array);

#endif
