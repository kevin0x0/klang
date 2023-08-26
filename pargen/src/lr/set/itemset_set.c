#include "pargen/include/lr/set/itemset_set.h"

#include <stdlib.h>

inline static size_t kev_itemsetset_hashing(KevItemSet* element) {
  KevItem* kitem = element->items;
  size_t hashval = 0;
  for (; kitem; kitem = kitem->next) {
    hashval += ((size_t)(kitem->rule) + kitem->dot) >> 3;
  }
  return hashval;
}

static void kev_itemsetset_rehash(KevItemSetSet* to, KevItemSetSet* from) {
  size_t from_capacity = from->capacity;
  size_t to_capacity = to->capacity;
  KevItemSetSetNode** from_array = from->array;
  KevItemSetSetNode** to_array = to->array;
  size_t mask = to_capacity - 1;
  for (size_t i = 0; i < from_capacity; ++i) {
    KevItemSetSetNode* node = from_array[i];
    while (node) {
      KevItemSetSetNode* tmp = node->next;
      size_t hash_val = kev_itemsetset_hashing(node->element);
      size_t index = hash_val & mask;
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

static bool kev_itemsetset_expand(KevItemSetSet* set) {
  KevItemSetSet new_set;
  if (!kev_itemsetset_init(&new_set, set->capacity << 1, set->equal))
    return false;
  kev_itemsetset_rehash(&new_set, set);
  *set = new_set;
  return true;
}

static void kev_itemsetset_bucket_free(KevItemSetSetNode* bucket) {
  while (bucket) {
    KevItemSetSetNode* tmp = bucket->next;
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

bool kev_itemsetset_init(KevItemSetSet* set, size_t capacity, bool (*equal)(KevItemSet*, KevItemSet*)) {
  if (!set) return false;

  capacity = pow_of_2_above(capacity);
  KevItemSetSetNode** array = (KevItemSetSetNode**)malloc(sizeof (KevItemSetSetNode*) * capacity);
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
  set->equal = equal;
  return true;
}

void kev_itemsetset_destroy(KevItemSetSet* set) {
  if (set) {
    KevItemSetSetNode** array = set->array;
    size_t capacity = set->capacity;
    for (size_t i = 0; i < capacity; ++i)
      kev_itemsetset_bucket_free(array[i]);
    free(array);
    set->array = NULL;
    set->capacity = 0;
    set->size = 0;
  }
}

KevItemSetSet* kev_itemsetset_create(size_t capacity, bool (*equal)(KevItemSet*, KevItemSet*)) {
  KevItemSetSet* iset_set = (KevItemSetSet*)malloc(sizeof (KevItemSetSet));
  if (!iset_set || !kev_itemsetset_init(iset_set, capacity, equal)) {
    kev_itemsetset_delete(iset_set);
    return NULL;
  }
  return iset_set;
}

bool kev_itemsetset_insert(KevItemSetSet* set, KevItemSet* element) {
  if (set->size >= set->capacity && !kev_itemsetset_expand(set))
    return false;

  KevItemSetSetNode* new_node = (KevItemSetSetNode*)malloc(sizeof (*new_node));
  if (!new_node) return false;

  size_t index = (set->capacity - 1) & kev_itemsetset_hashing(element);
  new_node->element = element;
  new_node->next = set->array[index];
  set->array[index] = new_node;
  set->size++;
  return true;
}

KevItemSetSetNode* kev_itemsetset_search(KevItemSetSet* set, KevItemSet* element) {
  size_t index = (set->capacity - 1) & kev_itemsetset_hashing(element);
  KevItemSetSetNode* node = set->array[index];
  while (node) {
    if (set->equal(element, node->element))
      break;
    node = node->next;
  }

  return node;
}

void kev_itemsetset_delete(KevItemSetSet* set) {
  kev_itemsetset_destroy(set);
  free(set);
}
