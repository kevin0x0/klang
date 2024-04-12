#ifndef KEVCC_UTILS_INCLUDE_SET_BITSET_H
#define KEVCC_UTILS_INCLUDE_SET_BITSET_H

#include "utils/include/general/global_def.h"
#include "utils/include/bits/bitop.h"
#include "utils/include/utils/utils.h"

#define KBITSET_MASK        (0x3F)
#define KBITSET_SHIFT       (6)
#define KBITSET_INTLEN      (64)

#define kbitset_ctz(x)      (kbit_ctz(x))
#define kbitset_popcount(x) (kbit_popcount(x))

typedef uint64_t KevBitSetInt;

typedef struct tagKBitSet {
  KevBitSetInt* bits;
  size_t length;
} KBitSet;

bool kbitset_init(KBitSet* bitset, size_t initial_size);
bool kbitset_init_copy(KBitSet* bitset, KBitSet* src);
void kbitset_destroy(KBitSet* bitset);
KBitSet* kbitset_create(size_t initial_size);
KBitSet* kbitset_create_copy(KBitSet* src);
void kbitset_delete(KBitSet* bitset);
bool kbitset_expand(KBitSet* bitset, size_t new_size);
bool kbitset_assign(KBitSet* bitset, KBitSet* src);
static inline void kbitset_make_empty(KBitSet* set);

static inline bool kbitset_set(KBitSet* bitset, size_t bit);
static inline void kbitset_clear(KBitSet* bitset, size_t bit);

bool kbitset_union(KBitSet* dest, KBitSet* src);
bool kbitset_intersection(KBitSet* dest, KBitSet* src);
bool kbitset_completion(KBitSet* bitset);
bool kbitset_difference(KBitSet* dest, KBitSet* src);
static inline bool kbitset_changed_after_shrinking_union(KBitSet* dest, KBitSet* src);

size_t kbitset_iter_next(KBitSet* bitset, size_t previous);
static inline size_t kbitset_iter_begin(KBitSet* bitset);

static inline bool kbitset_has_element(KBitSet* bitset, size_t bit);
bool kbitset_equal(KBitSet* set1, KBitSet* set2);
static inline bool kbitset_is_subset(KBitSet* set, KBitSet* superset);
static inline bool kbitset_empty(KBitSet* set);
size_t kbitset_size(KBitSet* bitset);
static inline size_t kbitset_capacity(KBitSet* bitset);

static inline bool kbitset_set(KBitSet* bitset, size_t bit) {
  KevBitSetInt i = bit >> KBITSET_SHIFT;
  if (k_unlikely(i >= bitset->length && !kbitset_expand(bitset, bit + 1))) {
    return false;
  }
  bitset->bits[i] |= ((KevBitSetInt)0x1 << (bit & KBITSET_MASK));
  return true;
}

static inline void kbitset_clear(KBitSet* bitset, size_t bit) {
  KevBitSetInt i = bit >> KBITSET_SHIFT;
  if (k_likely(i < bitset->length))
    bitset->bits[i] &= ~((KevBitSetInt)0x1 << (bit & KBITSET_MASK));
}

static inline bool kbitset_has_element(KBitSet* bitset, size_t bit) {
  KevBitSetInt i = bit >> KBITSET_SHIFT;
  if ((i >= bitset->length)) {
    return false;
  }
  return bitset->bits[i] & ((KevBitSetInt)0x1 << (bit & KBITSET_MASK));
}

static inline bool kbitset_is_subset(KBitSet* set, KBitSet* superset) {
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

static inline size_t kbitset_iter_begin(KBitSet* bitset) {
  return kbitset_has_element(bitset, 0) ? 0 : kbitset_iter_next(bitset, 0);
}

static inline bool kbitset_empty(KBitSet* set) {
  for (size_t i = 0; i < set->length; ++i) {
    if (set->bits[i]) return false;
  }
  return true;
}

static inline size_t kbitset_capacity(KBitSet* bitset) {
  return bitset->length << KBITSET_SHIFT;
}

static inline void kbitset_make_empty(KBitSet* set) {
  for (size_t i = 0; i < set->length; ++i) {
    set->bits[i] = 0;
  }
}

static inline bool kbitset_changed_after_shrinking_union(KBitSet* dest, KBitSet* src) {
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
