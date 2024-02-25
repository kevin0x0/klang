#include "utils/include/set/hashset.h"

#include <stdlib.h>

inline static size_t kev_hashset_hashing(void* element) {
  return (size_t)element >> 3;
}

static void kev_hashset_rehash(KevHashSet* to, KevHashSet* from) {
  size_t from_capacity = from->capacity;
  size_t to_capacity = to->capacity;
  KevHashSetNode** from_array = from->array;
  KevHashSetNode** to_array = to->array;
  size_t mask = to_capacity - 1;
  for (size_t i = 0; i < from_capacity; ++i) {
    KevHashSetNode* node = from_array[i];
    while (node) {
      KevHashSetNode* tmp = node->next;
      size_t hashval = kev_hashset_hashing(node->element);
      size_t index = hashval & mask;
      node->next = to_array[index];
      to_array[index] = node;
      node = tmp;
    }
  }
  to->size = from->size;
  free(from->array);
  from->array = NULL;
  from->capacity = 0;
  from->size = 0;
}

static bool kev_hashset_expand(KevHashSet* set) {
  KevHashSet new_set;
  if (!khashset_init(&new_set, set->capacity << 1))
    return false;
  kev_hashset_rehash(&new_set, set);
  *set = new_set;
  return true;
}

static void kev_hashset_bucket_free(KevHashSetNode* bucket) {
  while (bucket) {
    KevHashSetNode* tmp = bucket->next;
    free(bucket);
    bucket = tmp;
  }
}

inline static size_t pow_of_2_above(size_t num) {
  size_t pow = 8;
  while (pow < num)
    pow <<= 1;

  return pow;
}

bool khashset_init(KevHashSet* set, size_t capacity) {
  if (!set) return false;

  /* TODO: make sure capacity is power of 2 */
  capacity = pow_of_2_above(capacity);
  KevHashSetNode** array = (KevHashSetNode**)malloc(sizeof (KevHashSetNode*) * capacity);
  if (!array) {
    set->array = NULL;
    set->capacity = 0;
    set->size = 0;
    return false;
  }

  for (size_t i = 0; i < capacity; ++i) {
    array[i] = NULL;
  }
  
  set->array = array;
  set->capacity = capacity;
  set->size = 0;
  return true;
}

void khashset_destroy(KevHashSet* set) {
  if (set) {
    KevHashSetNode** array = set->array;
    size_t capacity = set->capacity;
    for (size_t i = 0; i < capacity; ++i)
      kev_hashset_bucket_free(array[i]);
    free(array);
    set->array = NULL;
    set->capacity = 0;
    set->size = 0;
  }
}

bool khashset_insert(KevHashSet* set, void* element) {
  if (set->size >= set->capacity && !kev_hashset_expand(set))
    return false;

  KevHashSetNode* new_node = (KevHashSetNode*)malloc(sizeof (*new_node));
  if (!new_node) return false;

  size_t index = (set->capacity - 1) & kev_hashset_hashing(element);
  new_node->element = element;
  new_node->next = set->array[index];
  set->array[index] = new_node;
  set->size++;
  return true;
}

bool khashset_has(KevHashSet* set, void* element) {
  size_t index = (set->capacity - 1) & kev_hashset_hashing(element);
  KevHashSetNode* node = set->array[index];
  for (; node; node = node->next) {
    if (node->element == element) break;
  }

  return node != NULL;
}
