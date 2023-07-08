#ifndef KEVCC_LEXGEN_INCLUDE_TABLEGEN_TRANS_TABLE_H
#define KEVCC_LEXGEN_INCLUDE_TABLEGEN_TRANS_TABLE_H

#include "lexgen/include/finite_automaton/finite_automaton.h"
#include "lexgen/include/general/global_def.h"

uint8_t (*kev_get_trans_256_u8(KevFA* dfa))[256];
uint8_t (*kev_get_trans_128_u8(KevFA* dfa))[128];
uint16_t (*kev_get_trans_256_u16(KevFA* dfa))[256];
uint16_t (*kev_get_trans_128_u16(KevFA* dfa))[128];

#endif
