#include "klang/include/code/klcontbl.h"


#include "klang/include/cst/klcst_expr.h"
#include "klang/include/parse/klstrtab.h"
#include <stdlib.h>
#include <string.h>

static inline size_t klcontbl_hashing(KlStrTab* strtab, KlConstant* con) {
  switch (con->type) {
    case KL_INT: {
      return (con->intval << 8) + con->intval;
    }
    case KL_STRING: {
      char* str = klstrtab_getstring(strtab, con->string.id);
      char* end = str + con->string.length;
      size_t hash = 0;
      while (str != end)
        hash = (*str++) + (hash << 6) + (hash << 16) - hash;
      return hash;
    }
    default: {
      kl_assert(false, "this type can not be inserted into constant table.");
      return 0;
    }
  }
}
static inline bool klcontbl_constant_equal(KlStrTab* strtab, KlConstant* con1, KlConstant* con2) {
  if (con1->type != con2->type) return false;
  switch (con1->type) {
    case KL_INT: {
      return con1->intval == con2->intval;
    }
    case KL_STRING: {
      return con1->string.length == con2->string.length &&
             0 == strncmp(klstrtab_getstring(strtab, con1->string.id),
                          klstrtab_getstring(strtab, con2->string.id), con1->string.length);
    }
    default: {
      kl_assert(false, "this type can not be inserted into constant table.");
      return false;
    }
  }
}

static void klcontbl_rehash(KlConTbl* to, KlConTbl* from) {
  size_t from_capacity = from->capacity;
  size_t to_capacity = to->capacity;
  KlConEntry** from_array = from->array;
  KlConEntry** to_array = to->array;
  size_t mask = to_capacity - 1;
  for (size_t i = 0; i < from_capacity; ++i) {
    KlConEntry* constant = from_array[i];
    while (constant) {
      KlConEntry* tmp = constant->next;
      size_t hashval = constant->hash;
      size_t index = hashval & mask;
      constant->next = to_array[index];
      to_array[index] = constant;
      constant = tmp;
    }
  }
  to->size = from->size;
  free(from->array);
  from->array = NULL;
  from->capacity = 0;
  from->size = 0;
}

static bool klcontbl_expand(KlConTbl* contbl) {
  KlConTbl new_map;
  if (kl_unlikely(!klcontbl_init(&new_map, contbl->capacity << 1, contbl->strtab)))
    return false;
  klcontbl_rehash(&new_map, contbl);
  *contbl = new_map;
  return true;
}

static void klcontbl_bucket_free(KlConEntry* bucket) {
  while (bucket) {
    KlConEntry* tmp = bucket->next;
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

bool klcontbl_init(KlConTbl* contbl, size_t capacity, KlStrTab* strtab) {
  if (!contbl) return false;

  /* TODO: make sure capacity is power of 2 */
  capacity = pow_of_2_above(capacity);
  KlConEntry** array = (KlConEntry**)malloc(sizeof (KlConEntry*) * capacity);
  if (kl_unlikely(!array)) {
    contbl->array = NULL;
    contbl->capacity = 0;
    contbl->size = 0;
    return false;
  }

  for (size_t i = 0; i < capacity; ++i) {
    array[i] = NULL;
  }
  
  contbl->array = array;
  contbl->capacity = capacity;
  contbl->size = 0;
  contbl->strtab = strtab;
  return true;
}

void klcontbl_destroy(KlConTbl* contbl) {
  if (kl_unlikely(!contbl)) return;

  KlConEntry** array = contbl->array;
  size_t capacity = contbl->capacity;
  for (size_t i = 0; i < capacity; ++i)
    klcontbl_bucket_free(array[i]);
  free(array);
  contbl->array = NULL;
  contbl->capacity = 0;
  contbl->size = 0;
}

KlConTbl* klcontbl_create(size_t capacity, KlStrTab* strtab) {
  KlConTbl* contbl = (KlConTbl*)malloc(sizeof (KlConTbl));
  if (kl_unlikely(!contbl || !klcontbl_init(contbl, capacity, strtab))) {
    free(strtab);
  }
  return contbl;
}

void klcontbl_delete(KlConTbl* contbl) {
  klcontbl_destroy(contbl);
  free(contbl);
}

KlConEntry* klcontbl_insert(KlConTbl* contbl, KlConstant* con) {
  if (kl_unlikely(contbl->size >= contbl->capacity && !klcontbl_expand(contbl)))
    return NULL;

  KlConEntry* newconentry = (KlConEntry*)malloc(sizeof (*newconentry));
  if (kl_unlikely(!newconentry)) return NULL;

  size_t hash = klcontbl_hashing(contbl->strtab, con);
  size_t index = (contbl->capacity - 1) & hash;
  newconentry->con = *con;
  newconentry->hash = hash;
  newconentry->index = contbl->size++;
  newconentry->next = contbl->array[index];
  contbl->array[index] = newconentry;
  return newconentry;
}

KlConEntry* klcontbl_search(KlConTbl* contbl, KlConstant* constant) {
  size_t hash = klcontbl_hashing(contbl->strtab, constant);
  size_t index = (contbl->capacity - 1) & hash;
  KlConEntry* conentry = contbl->array[index];
  for (; conentry; conentry = conentry->next) {
    if (hash == conentry->hash && klcontbl_constant_equal(contbl->strtab, constant, &conentry->con))
      return conentry;
  }

  return NULL;
}

KlConEntry* klcontbl_get(KlConTbl* contbl, KlConstant* constant) {
  size_t hash = klcontbl_hashing(contbl->strtab, constant);
  size_t index = (contbl->capacity - 1) & hash;
  KlConEntry* conentry = contbl->array[index];
  for (; conentry; conentry = conentry->next) {
    if (hash == conentry->hash && klcontbl_constant_equal(contbl->strtab, constant, &conentry->con))
      return conentry;
  }
  /* not found, insert */
  if (kl_unlikely(contbl->size >= contbl->capacity && !klcontbl_expand(contbl)))
    return NULL;
  KlConEntry* newconentry = (KlConEntry*)malloc(sizeof (*newconentry));
  if (kl_unlikely(!newconentry)) return false;

  newconentry->con = *constant;
  newconentry->hash = hash;
  newconentry->index = contbl->size++;
  newconentry->next = contbl->array[index];
  contbl->array[index] = newconentry;
  return newconentry;
}
