#include "lexgen/include/tablegen/acc_mapping.h"
#include "lexgen/include/general/global_def.h"
#include <stdlib.h>

int16_t* kev_get_acc_mapping_16(KevFA* dfa, uint64_t* acc_mapping_array) {
  size_t non_acc_number = kev_dfa_non_accept_state_number(dfa);
  size_t state_number = kev_dfa_accept_state_number(dfa) + non_acc_number;
  int16_t* mapping = (int16_t*)malloc(sizeof (int16_t) * state_number);
  if (!mapping) return NULL;
  for (size_t i = 0; i < non_acc_number; ++i) {
    mapping[i] = -1;
  }
  for (size_t i = non_acc_number; i < state_number; ++i) {
    mapping[i] = (int16_t)acc_mapping_array[i - non_acc_number];
  }
  return mapping;
}

int8_t* kev_get_acc_mapping_8(KevFA* dfa, uint64_t* acc_mapping_array) {
  size_t non_acc_number = kev_dfa_non_accept_state_number(dfa);
  size_t state_number = kev_dfa_accept_state_number(dfa) + non_acc_number;
  int8_t* mapping = (int8_t*)malloc(sizeof (int8_t) * state_number);
  if (!mapping) return NULL;
  for (size_t i = 0; i < non_acc_number; ++i) {
    mapping[i] = -1;
  }
  for (size_t i = non_acc_number; i < state_number; ++i) {
    mapping[i] = (int8_t)acc_mapping_array[i - non_acc_number];
  }
  return mapping;
}
