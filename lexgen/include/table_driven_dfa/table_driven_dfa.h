#ifndef KEVCC_LEXGEN_INCLUDE_TABLE_DRIVEN_DFA_TABLE_DRIVEN_DFA_H
#define KEVCC_LEXGEN_INCLUDE_TABLE_DRIVEN_DFA_TABLE_DRIVEN_DFA_H
#include "lexgen/include/finite_automaton/finite_automaton.h"
#include "lexgen/include/general/global_def.h"

uint8_t (*kev_get_table_256_u8(KevFA* dfa))[256];
uint8_t (*kev_get_table_128_u8(KevFA* dfa))[128];
uint16_t (*kev_get_table_256_u16(KevFA* dfa))[256];
uint16_t (*kev_get_table_128_u16(KevFA* dfa))[128];

#endif
