#ifndef KEVCC_LEXGEN_INCLUDE_FINITE_AUTOMATON_SET_BITSET_H
#define KEVCC_LEXGEN_INCLUDE_FINITE_AUTOMATON_SET_BITSET_H

#include "lexgen/include/general/global_def.h"

#define KEV_BITSET_MASK       (0x3F)
#define KEV_BITSET_SHIFT      (6)

typedef struct tagKevBitSet {
  uint64_t* bits;
  size_t length;
} KevBitSet;

bool kev_bitset_init(KevBitSet* bitset, size_t initial_size);
bool kev_bitset_init_copy(KevBitSet* bitset, KevBitSet* src);
void kev_bitset_destroy(KevBitSet* bitset);
KevBitSet* kev_bitset_create(size_t initial_size);
KevBitSet* kev_bitset_create_copy(KevBitSet* src);
void kev_bitset_delete(KevBitSet* bitset);
bool kev_bitset_expand(KevBitSet* bitset, size_t new_size);

static inline bool kev_bitset_set(KevBitSet* bitset, size_t bit);
static inline void kev_bitset_clear(KevBitSet* bitset, size_t bit);

bool kev_bitset_union(KevBitSet* dest, KevBitSet* src);
bool kev_bitset_intersection(KevBitSet* dest, KevBitSet* src);
bool kev_bitset_completion(KevBitSet* bitset);
bool kev_bitset_difference(KevBitSet* dest, KevBitSet* src);

size_t kev_bitset_iterate_next(KevBitSet* bitset, size_t previous);
static inline size_t kev_bitset_iterate_begin(KevBitSet* bitset);

static inline bool kev_bitset_has_element(KevBitSet* bitset, size_t bit);
bool kev_bitset_equal(KevBitSet* set1, KevBitSet* set2);
static inline bool kev_bitset_empty(KevBitSet* set);
size_t kev_bitset_size(KevBitSet* bitset);
static inline size_t kev_bitset_capacity(KevBitSet* bitset);

static inline bool kev_bitset_set(KevBitSet* bitset, size_t bit) {
  uint64_t i = bit >> KEV_BITSET_SHIFT;
  if (i >= bitset->length && !kev_bitset_expand(bitset, bit + 1)) {
    return false;
  }
  bitset->bits[i] |= ((uint64_t)0x1 << (bit & KEV_BITSET_MASK));
  return true;
}

static inline void kev_bitset_clear(KevBitSet* bitset, size_t bit) {
  uint64_t i = bit >> KEV_BITSET_SHIFT;
  if (i < bitset->length)
    bitset->bits[i] &= ~((uint64_t)0x1 << (bit & KEV_BITSET_MASK));
}

static inline bool kev_bitset_has_element(KevBitSet* bitset, size_t bit) {
  uint64_t i = bit >> KEV_BITSET_SHIFT;
  if (i >= bitset->length) {
    return false;
  }
  return bitset->bits[i] & ((uint64_t)0x1 << (bit & KEV_BITSET_MASK));
}

static inline size_t kev_bitset_iterate_begin(KevBitSet* bitset) {
  return kev_bitset_has_element(bitset, 0) ? 0 : kev_bitset_iterate_next(bitset, 0);
}

static inline bool kev_bitset_empty(KevBitSet* set) {
  for (size_t i = 0; i < set->length; ++i) {
    if (set->bits[i]) return false;
  }
  return true;
}

static inline size_t kev_bitset_capacity(KevBitSet* bitset) {
  return bitset->length << KEV_BITSET_SHIFT;
}

#endif
