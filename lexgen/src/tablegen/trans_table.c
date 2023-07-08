#include "lexgen/include/tablegen/trans_table.h"
#include "lexgen/include/finite_automaton/finite_automaton.h"
#include "lexgen/include/finite_automaton/graph.h"

#include <stdlib.h>

uint8_t (*kev_get_trans_256_u8(KevFA* dfa))[256] {
  size_t state_number = kev_fa_state_assign_id(dfa, 0);
  if (state_number >= 256) return NULL;
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
      if ((uint64_t)edge->attr <= 255)
        table[id][(uint8_t)edge->attr] = (uint8_t)edge->node->id;
      edge = edge->next;
    }
    node = node->next;
  }
  return table;
}

uint8_t (*kev_get_trans_128_u8(KevFA* dfa))[128] {
  size_t state_number = kev_fa_state_assign_id(dfa, 0);
  if (state_number >= 256) return NULL;
  uint8_t (*table)[128] = (uint8_t(*)[128])malloc(sizeof (*table) * state_number);
  if (!table) return NULL;
  for (size_t i = 0; i < state_number; ++i) {
    for (size_t j = 0; j < 128; ++j) {
      table[i][j] = 127;
    }
  }
  KevGraphNode* node = kev_fa_get_states(dfa);
  while (node) {
    KevGraphEdge* edge = kev_graphnode_get_edges(node);
    uint64_t id = node->id;
    while (edge) {
      if ((uint64_t)edge->attr <= 127)
        table[id][(uint8_t)edge->attr] = (uint8_t)edge->node->id;
      edge = edge->next;
    }
    node = node->next;
  }
  return table;
}

uint16_t (*kev_get_trans_256_u16(KevFA* dfa))[256] {
  size_t state_number = kev_fa_state_assign_id(dfa, 0);
  if (state_number >= 65536) return NULL;
  uint16_t (*table)[256] = (uint16_t(*)[256])malloc(sizeof (*table) * state_number);
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
      if ((uint64_t)edge->attr <= 255)
        table[id][(uint16_t)edge->attr] = (uint16_t)edge->node->id;
      edge = edge->next;
    }
    node = node->next;
  }
  return table;
}

uint16_t (*kev_get_trans_128_u16(KevFA* dfa))[128] {
  size_t state_number = kev_fa_state_assign_id(dfa, 0);
  if (state_number >= 65536) return NULL;
  uint16_t (*table)[128] = (uint16_t(*)[128])malloc(sizeof (*table) * state_number);
  if (!table) return NULL;
  for (size_t i = 0; i < state_number; ++i) {
    for (size_t j = 0; j < 128; ++j) {
      table[i][j] = 127;
    }
  }
  KevGraphNode* node = kev_fa_get_states(dfa);
  while (node) {
    KevGraphEdge* edge = kev_graphnode_get_edges(node);
    uint64_t id = node->id;
    while (edge) {
      if ((uint64_t)edge->attr <= 127)
        table[id][(uint16_t)edge->attr] = (uint16_t)edge->node->id;
      edge = edge->next;
    }
    node = node->next;
  }
  return table;
}
