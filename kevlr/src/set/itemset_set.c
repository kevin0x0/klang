#include "kevlr/include/set/itemset_set.h"

#include <stdlib.h>

inline static size_t klr_itemsetset_hashing(KlrItemSet* element) {
  KlrItem* kitem = element->items;
  size_t hashval = 0;
  for (; kitem; kitem = kitem->next) {
    hashval += ((size_t)(kitem->rule) + kitem->dot) >> 3;
  }
  return hashval;
}

static void klr_itemsetset_rehash(KlrItemSetSet* to, KlrItemSetSet* from) {
  size_t from_capacity = from->capacity;
  size_t to_capacity = to->capacity;
  KlrItemSetSetNode** from_array = from->array;
  KlrItemSetSetNode** to_array = to->array;
  size_t mask = to_capacity - 1;
  for (size_t i = 0; i < from_capacity; ++i) {
    KlrItemSetSetNode* node = from_array[i];
    while (node) {
      KlrItemSetSetNode* tmp = node->next;
      size_t hashval = node->hashval;
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

static bool klr_itemsetset_expand(KlrItemSetSet* set) {
  KlrItemSetSet new_set;
  if (!klr_itemsetset_init(&new_set, set->capacity << 1, set->equal))
    return false;
  klr_itemsetset_rehash(&new_set, set);
  *set = new_set;
  return true;
}

static void klr_itemsetset_bucket_free(KlrItemSetSetNode* bucket) {
  while (bucket) {
    KlrItemSetSetNode* tmp = bucket->next;
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

bool klr_itemsetset_init(KlrItemSetSet* set, size_t capacity, bool (*equal)(KlrItemSet*, KlrItemSet*)) {
  if (!set) return false;

  capacity = pow_of_2_above(capacity);
  KlrItemSetSetNode** array = (KlrItemSetSetNode**)malloc(sizeof (KlrItemSetSetNode*) * capacity);
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

void klr_itemsetset_destroy(KlrItemSetSet* set) {
  if (set) {
    KlrItemSetSetNode** array = set->array;
    size_t capacity = set->capacity;
    for (size_t i = 0; i < capacity; ++i)
      klr_itemsetset_bucket_free(array[i]);
    free(array);
    set->array = NULL;
    set->capacity = 0;
    set->size = 0;
  }
}

KlrItemSetSet* klr_itemsetset_create(size_t capacity, bool (*equal)(KlrItemSet*, KlrItemSet*)) {
  KlrItemSetSet* iset_set = (KlrItemSetSet*)malloc(sizeof (KlrItemSetSet));
  if (!iset_set || !klr_itemsetset_init(iset_set, capacity, equal)) {
    klr_itemsetset_delete(iset_set);
    return NULL;
  }
  return iset_set;
}

bool klr_itemsetset_insert(KlrItemSetSet* set, KlrItemSet* element) {
  if (set->size >= set->capacity && !klr_itemsetset_expand(set))
    return false;

  KlrItemSetSetNode* new_node = (KlrItemSetSetNode*)malloc(sizeof (*new_node));
  if (!new_node) return false;

  size_t hashval = klr_itemsetset_hashing(element);
  size_t index = (set->capacity - 1) & hashval;
  new_node->element = element;
  new_node->hashval = hashval;
  new_node->next = set->array[index];
  set->array[index] = new_node;
  set->size++;
  return true;
}

KlrItemSetSetNode* klr_itemsetset_search(KlrItemSetSet* set, KlrItemSet* element) {
  size_t hashval = klr_itemsetset_hashing(element);
  size_t index = (set->capacity - 1) & hashval;
  KlrItemSetSetNode* node = set->array[index];
  while (node) {
    if (node->hashval == hashval &&
        set->equal(element, node->element))
      break;
    node = node->next;
  }

  return node;
}

void klr_itemsetset_delete(KlrItemSetSet* set) {
  klr_itemsetset_destroy(set);
  free(set);
}
