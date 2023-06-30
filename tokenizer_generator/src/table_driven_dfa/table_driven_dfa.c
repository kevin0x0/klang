#include "tokenizer_generator/include/table_driven_dfa/table_driven_dfa.h"
#include "tokenizer_generator/include/finite_automaton/finite_automaton.h"
#include "tokenizer_generator/include/finite_automaton/graph.h"
#include <stdlib.h>

uint8_t (*kev_get_table_256_u8(KevFA* dfa))[256] {
  size_t state_number = kev_fa_state_assign_id(dfa, 0);
  uint8_t (*table)[256] = (uint8_t(*)[256])malloc(sizeof (*table) * state_number);
  if (!table) return NULL;
  for (size_t i = 0; i < state_number; ++i) {
    for (size_t j = 0; j < 256; ++j) {
      table[i][j] = 255;
    }
  }
  KevGraphNode* node = kev_fa_get_states(dfa);
  while (node) {
    KevGraphEdge* edge = kev_graphnode_get_edges(node);
    uint64_t id = node->id;
    while (edge) {
      table[id][(uint8_t)edge->attr] = (uint8_t)edge->node->id;
      edge = edge->next;
    }
    node = node->next;
  }
  return table;
}
