#ifndef KEVCC_TOKENIZER_GENERATOR_INCLUDE_FINITE_AUTOMATA_BITSET_BITSET_H
#define KEVCC_TOKENIZER_GENERATOR_INCLUDE_FINITE_AUTOMATA_BITSET_BITSET_H

#include "tokenizer_generator/include/general/int_type.h"


#define KEV_BITSET_MASK       (0x3F)
#define KEV_BITSET_SHIFT      (6)


typedef struct tagKevBitSet {
  uint64_t* bits;
  uint64_t length;
} KevBitSet;

bool kev_bitset_init(KevBitSet* bitset, uint64_t initial_size);
bool kev_bitset_init_copy(KevBitSet* bitset, KevBitSet* src);
void kev_bitset_destroy(KevBitSet* bitset);
KevBitSet* kev_bitset_create(uint64_t initial_size);
KevBitSet* kev_bitset_create_copy(KevBitSet* src);
void kev_bitset_delete(KevBitSet* bitset);
bool kev_bitset_expand(KevBitSet* bitset, uint64_t new_size);

static inline bool kev_bitset_set(KevBitSet* bitset, uint64_t bit);
static inline void kev_bitset_clear(KevBitSet* bitset, uint64_t bit);

bool kev_bitset_union(KevBitSet* dest, KevBitSet* src);
bool kev_bitset_intersection(KevBitSet* dest, KevBitSet* src);
bool kev_bitset_completion(KevBitSet* bitset);

uint64_t kev_bitset_iterate_next(KevBitSet* bitset, uint64_t previous);
static inline uint64_t kev_bitset_iterate_begin(KevBitSet* bitset);

static inline bool kev_bitset_has_element(KevBitSet* bitset, uint64_t bit);
bool kev_bitset_equal(KevBitSet* set1, KevBitSet* set2);
static inline bool kev_bitset_empty(KevBitSet* set);


static inline bool kev_bitset_set(KevBitSet* bitset, uint64_t bit) {
  if (!bitset) return false;

  uint64_t i = bit >> KEV_BITSET_SHIFT;
  if (i >= bitset->length && !kev_bitset_expand(bitset, bit + 1)) {
    return false;
  }
  bitset->bits[i] |= ((uint64_t)0x1 << (bit & KEV_BITSET_MASK));
  return true;
}

static inline void kev_bitset_clear(KevBitSet* bitset, uint64_t bit) {
  if (!bitset) return;
  uint64_t i = bit >> KEV_BITSET_SHIFT;
  if (i < bitset->length)
    bitset->bits[i] &= ~((uint64_t)0x1 << (bit & KEV_BITSET_MASK));
}

static inline bool kev_bitset_has_element(KevBitSet* bitset, uint64_t bit) {
  if (!bitset) return false;

  uint64_t i = bit >> KEV_BITSET_SHIFT;
  if (i >= bitset->length) {
    return false;
  }
  return bitset->bits[i] & ((uint64_t)0x1 << (bit & KEV_BITSET_MASK));
}

static inline uint64_t kev_bitset_iterate_begin(KevBitSet* bitset) {
  return kev_bitset_has_element(bitset, 0) ? 0 : kev_bitset_iterate_next(bitset, 0);
}

static inline bool kev_bitset_empty(KevBitSet* set) {
  if (!set) return true;
  for (uint64_t i = 0; i < set->length; ++i) {
    if (set->bits[i]) return false;
  }
  return true;
}
#endif
