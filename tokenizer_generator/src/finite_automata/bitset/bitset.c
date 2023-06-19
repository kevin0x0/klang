#include "tokenizer_generator/include/finite_automata/bitset/bitset.h"
#include "tokenizer_generator/include/general/int_type.h"

#include <stdlib.h>

#define FIND_BIT_MASK32       ((uint64_t)0xFFFFFFFF)
#define FIND_BIT_MASK16       ((uint64_t)0xFFFF);
#define FIND_BIT_MASK8        ((uint64_t)0xFF);
#define FIND_BIT_MASK4        ((uint64_t)0xF);

bool kev_bitset_expand(KevBitSet* bitset, uint64_t new_size) {
  new_size = new_size & KEV_BITSET_MASK ? (new_size >> KEV_BITSET_SHIFT) + 1 : new_size >> KEV_BITSET_SHIFT;
  if (new_size > bitset->length) {
    uint64_t* new_bits = (uint64_t*)realloc(bitset->bits, sizeof (uint64_t) * new_size);
    if (!new_bits) return false;
    for (uint64_t i = bitset->length; i < new_size; ++i)
      new_bits[i] = 0;
    bitset->bits = new_bits;
    bitset->length = new_size;
  }
  return true;
}

bool kev_bitset_init(KevBitSet* bitset, uint64_t initial_size) {
  if (!bitset) return false;
  if (sizeof (uint64_t) != 8) {
    bitset->bits = NULL;
    bitset->length = 0;
    return false;
  }

  initial_size = initial_size & KEV_BITSET_MASK ? (initial_size >> KEV_BITSET_SHIFT) + 1 : initial_size >> KEV_BITSET_SHIFT;
  uint64_t* bits = (uint64_t*)malloc(sizeof (uint64_t) * initial_size);
  bitset->bits = bits;
  if (!bits) {
    bitset->length = 0;
    return false;
  }
  bitset->length = initial_size;
  for (uint64_t i = 0; i < initial_size; ++i)
    bits[i] = 0;
  return true;
}

bool kev_bitset_init_copy(KevBitSet* bitset, KevBitSet* src) {
  if (!bitset) return false;
  if (!src || sizeof (uint64_t) != 8) {
    bitset->bits = NULL;
    bitset->length = 0;
    return false;
  }

  uint64_t length = src->length;
  uint64_t* bits = (uint64_t*)malloc(sizeof (uint64_t) * length);
  bitset->bits = bits;
  if (!bits) {
    bitset->length = 0;
    return false;
  }

  uint64_t* src_bits = src->bits;
  for (uint64_t i = 0; i < length; ++i)
    bits[i] = src_bits[i];
  bitset->length = length;
  return true;
}

void kev_bitset_destroy(KevBitSet* bitset) {
  if (bitset) {
    free(bitset->bits);
    bitset->length = 0;
    bitset->bits = NULL;
  }
}

KevBitSet* kev_bitset_create(uint64_t initial_size) {
  KevBitSet* bitset = (KevBitSet*)malloc(sizeof (KevBitSet));
  if (!bitset || !kev_bitset_init(bitset, initial_size)) {
    kev_bitset_destroy(bitset);
    free(bitset);
    return NULL;
  }
  return bitset;
}

KevBitSet* kev_bitset_create_copy(KevBitSet* src) {
  KevBitSet* bitset = (KevBitSet*)malloc(sizeof (KevBitSet));
  if (!bitset || !kev_bitset_init_copy(bitset, src)) {
    kev_bitset_destroy(bitset);
    free(bitset);
    return NULL;
  }
  return bitset;
}

void kev_bitset_delete(KevBitSet* bitset) {
  kev_bitset_destroy(bitset);
  free(bitset);
}

bool kev_bitset_intersection(KevBitSet* dest, KevBitSet* src) {
  uint64_t min_len = dest->length > src->length ? src->length : dest->length;
  uint64_t i = 0;
  uint64_t* dest_bits = dest->bits;
  uint64_t* src_bits = src->bits;
  while (i < min_len) {
    dest_bits[i] &= src_bits[i];
    ++i;
  }
  uint64_t dest_size = dest->length;
  while (i < dest_size) {
    dest_bits[i] = 0;
  }
  return true;
}

bool kev_bitset_union(KevBitSet* dest, KevBitSet* src) {
  if (dest->length < src->length && !kev_bitset_expand(dest, src->length << KEV_BITSET_SHIFT)) {
    return false;
  }

  uint64_t len = src->length;
  uint64_t* dest_bits = dest->bits;
  uint64_t* src_bits = src->bits;
  for (uint64_t i = 0; i < len; ++i)
    dest_bits[i] |= src_bits[i];
  return true;
}

bool kev_bitset_completion(KevBitSet* bitset) {
  uint64_t len = bitset->length;
  uint64_t* bits = bitset->bits;
  for (uint64_t i = 0; i < len; ++i)
    bits[i] = ~bits[i];
  return true;
}

bool kev_bitset_equal(KevBitSet* set1, KevBitSet* set2) {
  uint64_t min_length = set1->length < set2->length ? set1->length : set2->length;
  for (uint64_t i = 0; i < min_length; ++i) {
    if (set1->bits[i] != set2->bits[i])
      return false;
  }
  KevBitSet* the_larger_set = set1->length < set2->length ? set1 : set2;
  for (uint64_t i = min_length; i < the_larger_set->length; ++i) {
    if (the_larger_set->bits[i] != 0)
      return false;
  }
  return true;
}

static inline uint8_t kev_bitset_find_first_bit(uint64_t bits) {
  uint64_t mask = FIND_BIT_MASK32;
  uint8_t current_pos = 0;
  uint8_t half_size = 32;
  while (half_size) {
    if (!(bits & mask)) {
      current_pos += half_size;
      bits >>= half_size;
    }
    half_size >>= 1;
    mask >>= half_size;
  }
  return current_pos;
}

uint64_t kev_bitset_iterate_next(KevBitSet* bitset, uint64_t previous) {
  uint64_t index = (previous + 1) >> KEV_BITSET_SHIFT;
  if (index >= bitset->length) return previous;
  uint64_t bit = (previous + 1) & KEV_BITSET_MASK;
  uint64_t* bits = bitset->bits;
  if (bits[index] >> bit)
    return 64 * index + bit + kev_bitset_find_first_bit(bits[index] >> bit);

  for (uint64_t i = index + 1; i < bitset->length; ++i) {
    if (bits[i] != 0)
      return 64 * i + kev_bitset_find_first_bit(bits[i]);
  }
  return previous;
}
