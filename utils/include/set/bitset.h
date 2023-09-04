#ifndef KEVCC_UTILS_INCLUDE_SET_BITSET_H
#define KEVCC_UTILS_INCLUDE_SET_BITSET_H

#include "utils/include/general/global_def.h"

#define KEV_BITSET_MASK       (0x3F)
#define KEV_BITSET_SHIFT      (6)
typedef uint64_t KevBitSetInt;

typedef struct tagKevBitSet {
  KevBitSetInt* bits;
  size_t length;
} KevBitSet;

bool kev_bitset_init(KevBitSet* bitset, size_t initial_size);
bool kev_bitset_init_copy(KevBitSet* bitset, KevBitSet* src);
void kev_bitset_destroy(KevBitSet* bitset);
KevBitSet* kev_bitset_create(size_t initial_size);
KevBitSet* kev_bitset_create_copy(KevBitSet* src);
void kev_bitset_delete(KevBitSet* bitset);
bool kev_bitset_expand(KevBitSet* bitset, size_t new_size);
bool kev_bitset_assign(KevBitSet* bitset, KevBitSet* src);
static inline void kev_bitset_make_empty(KevBitSet* set);

static inline bool kev_bitset_set(KevBitSet* bitset, size_t bit);
static inline void kev_bitset_clear(KevBitSet* bitset, size_t bit);

bool kev_bitset_union(KevBitSet* dest, KevBitSet* src);
bool kev_bitset_intersection(KevBitSet* dest, KevBitSet* src);
bool kev_bitset_completion(KevBitSet* bitset);
bool kev_bitset_difference(KevBitSet* dest, KevBitSet* src);
static inline bool kev_bitset_changed_after_shrinking_union(KevBitSet* dest, KevBitSet* src);

size_t kev_bitset_iterate_next(KevBitSet* bitset, size_t previous);
static inline size_t kev_bitset_iterate_begin(KevBitSet* bitset);

static inline bool kev_bitset_has_element(KevBitSet* bitset, size_t bit);
bool kev_bitset_equal(KevBitSet* set1, KevBitSet* set2);
static inline bool kev_bitset_is_subset(KevBitSet* set, KevBitSet* superset);
static inline bool kev_bitset_empty(KevBitSet* set);
size_t kev_bitset_size(KevBitSet* bitset);
static inline size_t kev_bitset_capacity(KevBitSet* bitset);

static inline bool kev_bitset_set(KevBitSet* bitset, size_t bit) {
  KevBitSetInt i = bit >> KEV_BITSET_SHIFT;
  if (i >= bitset->length && !kev_bitset_expand(bitset, bit + 1)) {
    return false;
  }
  bitset->bits[i] |= ((KevBitSetInt)0x1 << (bit & KEV_BITSET_MASK));
  return true;
}

static inline void kev_bitset_clear(KevBitSet* bitset, size_t bit) {
  KevBitSetInt i = bit >> KEV_BITSET_SHIFT;
  if (i < bitset->length)
    bitset->bits[i] &= ~((KevBitSetInt)0x1 << (bit & KEV_BITSET_MASK));
}

static inline bool kev_bitset_has_element(KevBitSet* bitset, size_t bit) {
  KevBitSetInt i = bit >> KEV_BITSET_SHIFT;
  if (i >= bitset->length) {
    return false;
  }
  return bitset->bits[i] & ((KevBitSetInt)0x1 << (bit & KEV_BITSET_MASK));
}

static inline bool kev_bitset_is_subset(KevBitSet* set, KevBitSet* superset) {
  size_t len = set->length < superset->length ? set->length : superset->length;
  for (size_t i = 0; i < len; ++i) {
    if ((set->bits[i] & superset->bits[i]) != set->bits[i])
      return false;
  }
  if (set->length > superset->length) {
    for (size_t i = len; i < set->length; ++i) {
      if (set->bits[i] != 0) return false;
    }
  }
  return true;
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

static inline void kev_bitset_make_empty(KevBitSet* set) {
  for (size_t i = 0; i < set->length; ++i) {
    set->bits[i] = 0;
  }
}

static inline bool kev_bitset_changed_after_shrinking_union(KevBitSet* dest, KevBitSet* src) {
  size_t len = src->length < dest->length ? src->length : dest->length;
  KevBitSetInt* dest_bits = dest->bits;
  KevBitSetInt* src_bits = src->bits;
  KevBitSetInt ret = 0;
  for (size_t i = 0; i < len; ++i) {
    KevBitSetInt tmp = dest_bits[i];
    /* if dest_bits[i] is not changed, the right value will be 0, else non-zero. */
    ret |= tmp ^ (dest_bits[i] |= src_bits[i]);
  }
  /* ret is not zero if and only if the dest_bits changed */
  return ret != 0;
}

#endif
