#include "utils/include/set/bitset.h"
#include "utils/include/utils/utils.h"

#include <stdlib.h>

bool kbitset_expand(KBitSet* bitset, size_t new_size) {
  new_size = new_size & KBITSET_MASK ? (new_size >> KBITSET_SHIFT) + 1 : new_size >> KBITSET_SHIFT;
  if (new_size > bitset->length) {
    KevBitSetInt* new_bits = (KevBitSetInt*)realloc(bitset->bits, sizeof (KevBitSetInt) * new_size);
    if (!new_bits) return false;
    for (size_t i = bitset->length; i < new_size; ++i)
      new_bits[i] = 0;
    bitset->bits = new_bits;
    bitset->length = new_size;
  }
  return true;
}

inline static size_t kbitset_bit_count(KevBitSetInt word) {
  //word = word - ((word >> 1) & 0x5555555555555555);
  //word = (word & 0x3333333333333333) + ((word >> 2) & 0x3333333333333333);
  //word = (word + (word >> 4)) & 0x0f0f0f0f0f0f0f0f;
  //word = word + (word >> 8);
  //word = word + (word >> 16);
  //word = word + (word >> 32);
  //return word & 0x7f;
  return kbitset_popcount(word);
}

bool kbitset_init(KBitSet* bitset, size_t initial_size) {
  if (k_unlikely(!bitset)) return false;
  initial_size = initial_size & KBITSET_MASK ? (initial_size >> KBITSET_SHIFT) + 1 : initial_size >> KBITSET_SHIFT;
  KevBitSetInt* bits = (KevBitSetInt*)malloc(sizeof (KevBitSetInt) * initial_size);
  bitset->bits = bits;
  if (k_unlikely(!bits)) {
    bitset->length = 0;
    return false;
  }
  bitset->length = initial_size;
  for (size_t i = 0; i < initial_size; ++i)
    bits[i] = 0;
  return true;
}

bool kbitset_init_copy(KBitSet* bitset, KBitSet* src) {
  if (k_unlikely(!bitset)) return false;
  if (k_unlikely(!src || sizeof (KevBitSetInt) != 8)) {
    bitset->bits = NULL;
    bitset->length = 0;
    return false;
  }

  size_t length = src->length;
  KevBitSetInt* bits = (KevBitSetInt*)malloc(sizeof (KevBitSetInt) * length);
  bitset->bits = bits;
  if (k_unlikely(!bits)) {
    bitset->length = 0;
    return false;
  }

  KevBitSetInt* src_bits = src->bits;
  for (size_t i = 0; i < length; ++i)
    bits[i] = src_bits[i];
  bitset->length = length;
  return true;
}

void kbitset_destroy(KBitSet* bitset) {
  if (k_likely(bitset)) {
    free(bitset->bits);
    bitset->length = 0;
    bitset->bits = NULL;
  }
}

KBitSet* kbitset_create(size_t initial_size) {
  KBitSet* bitset = (KBitSet*)malloc(sizeof (KBitSet));
  if (k_unlikely(!bitset || !kbitset_init(bitset, initial_size))) {
    kbitset_destroy(bitset);
    free(bitset);
    return NULL;
  }
  return bitset;
}

KBitSet* kbitset_create_copy(KBitSet* src) {
  KBitSet* bitset = (KBitSet*)malloc(sizeof (KBitSet));
  if (k_unlikely(!bitset || !kbitset_init_copy(bitset, src))) {
    kbitset_destroy(bitset);
    free(bitset);
    return NULL;
  }
  return bitset;
}

void kbitset_delete(KBitSet* bitset) {
  if (k_unlikely(!bitset)) return;
  free(bitset->bits);
  free(bitset);
}

bool kbitset_intersection(KBitSet* dest, KBitSet* src) {
  size_t min_len = dest->length > src->length ? src->length : dest->length;
  size_t i = 0;
  KevBitSetInt* dest_bits = dest->bits;
  KevBitSetInt* src_bits = src->bits;
  while (i < min_len) {
    dest_bits[i] &= src_bits[i];
    ++i;
  }
  size_t dest_size = dest->length;
  while (i < dest_size) {
    dest_bits[i] = 0;
  }
  return true;
}

bool kbitset_union(KBitSet* dest, KBitSet* src) {
  if (k_unlikely(dest->length < src->length && !kbitset_expand(dest, src->length << KBITSET_SHIFT))) {
    return false;
  }

  size_t len = src->length;
  KevBitSetInt* dest_bits = dest->bits;
  KevBitSetInt* src_bits = src->bits;
  for (size_t i = 0; i < len; ++i)
    dest_bits[i] |= src_bits[i];
  return true;
}

bool kbitset_completion(KBitSet* bitset) {
  size_t len = bitset->length;
  KevBitSetInt* bits = bitset->bits;
  for (size_t i = 0; i < len; ++i)
    bits[i] = ~bits[i];
  return true;
}

bool kbitset_difference(KBitSet* dest, KBitSet* src) {
  size_t len = src->length < dest->length ? src->length : dest->length;
  KevBitSetInt* dest_bits = dest->bits;
  KevBitSetInt* src_bits = src->bits;
  for (size_t i = 0; i < len; ++i)
    dest_bits[i] &= ~src_bits[i];
  return true;
}

bool kbitset_assign(KBitSet* bitset, KBitSet* src) {
  if (bitset->length < src->length) {
    KevBitSetInt* buf = (KevBitSetInt*)malloc(sizeof (KevBitSetInt) * src->length);
    if (k_unlikely(!buf)) return false;
    bitset->bits = buf;
    bitset->length = src->length;
  }
  KevBitSetInt* bits = bitset->bits;
  KevBitSetInt* src_bits = src->bits;
  size_t len = src->length;
  for (size_t i = 0; i < len; ++i)
    bits[i] = src_bits[i];
  return true;
}

bool kbitset_equal(KBitSet* set1, KBitSet* set2) {
  size_t min_length = set1->length < set2->length ? set1->length : set2->length;
  for (size_t i = 0; i < min_length; ++i) {
    if (set1->bits[i] != set2->bits[i])
      return false;
  }
  KBitSet* the_larger_set = set1->length < set2->length ? set2 : set1;
  for (size_t i = min_length; i < the_larger_set->length; ++i) {
    if (the_larger_set->bits[i] != 0)
      return false;
  }
  return true;
}

//static inline uint8_t kbitset_find_first_bit(size_t bits) {
//  uint8_t current_pos = 0;
//  KevBitSetInt mask = 0xFFFFFFFF;
//  uint8_t half = 32;
//  while (half) {
//    if (!(bits & mask)) {
//      current_pos += half;
//      bits >>= half;
//    }
//    half >>= 1;
//    mask >>= half;
//  }
//  return current_pos;
//}
//static inline uint8_t kbitset_find_first_bit(KevBitSetInt bits) {
//  KevBitSetInt allone = ~(bits & (bits - 1));
//  return kbitset_bit_count(allone);
//}

size_t kbitset_iter_next(KBitSet* bitset, size_t previous) {
  size_t index = (previous + 1) >> KBITSET_SHIFT;
  if (index >= bitset->length) return previous;
  size_t next = (previous + 1) & KBITSET_MASK;
  KevBitSetInt* bits = bitset->bits;
  if (bits[index] >> next)
    return previous + 1 + kbitset_ctz(bits[index] >> next);

  for (size_t i = index + 1; i < bitset->length; ++i) {
    if (bits[i])
      return KBITSET_INTLEN * i + kbitset_ctz(bits[i]);
  }
  return previous;
}

size_t kbitset_size(KBitSet* bitset) {
  size_t count = 0;
  for (size_t i = 0; i < bitset->length; ++i) {
    if (bitset->bits[i])
      count += kbitset_bit_count(bitset->bits[i]);
  }
  return count;
}
