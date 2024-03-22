#include "klang/include/code/klsymtbl.h"
#include "klang/include/parse/klstrtab.h"
#include <stdlib.h>
#include <string.h>

inline static size_t klsymtbl_hashing(KlStrTab* strtab, KlStrDesc name) {
  char* str = klstrtab_getstring(strtab, name.id);
  char* end = str + name.length;
  size_t hash = 0;
  while (str != end)
    hash = (*str++) + (hash << 6) + (hash << 16) - hash;
  return hash;
}

static void klsymtbl_rehash(KlSymTbl* to, KlSymTbl* from) {
  size_t from_capacity = from->capacity;
  size_t to_capacity = to->capacity;
  KlSymbol** from_array = from->array;
  KlSymbol** to_array = to->array;
  size_t mask = to_capacity - 1;
  for (size_t i = 0; i < from_capacity; ++i) {
    KlSymbol* symbol = from_array[i];
    while (symbol) {
      KlSymbol* tmp = symbol->next;
      size_t hashval = symbol->hash;
      size_t index = hashval & mask;
      symbol->next = to_array[index];
      to_array[index] = symbol;
      symbol = tmp;
    }
  }
  to->size = from->size;
  free(from->array);
  from->array = NULL;
  from->capacity = 0;
  from->size = 0;
}

static bool klsymtbl_expand(KlSymTbl* symtbl) {
  KlSymTbl new_map;
  if (kl_unlikely(!klsymtbl_init(&new_map, symtbl->capacity << 1, symtbl->strtab)))
    return false;
  klsymtbl_rehash(&new_map, symtbl);
  *symtbl = new_map;
  return true;
}

static void klsymtbl_bucket_free(KlSymbol* bucket) {
  while (bucket) {
    KlSymbol* tmp = bucket->next;
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

bool klsymtbl_init(KlSymTbl* symtbl, size_t capacity, KlStrTab* strtab) {
  if (!symtbl) return false;

  /* TODO: make sure capacity is power of 2 */
  capacity = pow_of_2_above(capacity);
  KlSymbol** array = (KlSymbol**)malloc(sizeof (KlSymbol*) * capacity);
  if (kl_unlikely(!array)) {
    symtbl->array = NULL;
    symtbl->capacity = 0;
    symtbl->size = 0;
    return false;
  }

  for (size_t i = 0; i < capacity; ++i) {
    array[i] = NULL;
  }
  
  symtbl->array = array;
  symtbl->capacity = capacity;
  symtbl->size = 0;
  symtbl->strtab = strtab;
  return true;
}

void klsymtbl_destroy(KlSymTbl* map) {
  if (kl_unlikely(!map)) return;

  KlSymbol** array = map->array;
  size_t capacity = map->capacity;
  for (size_t i = 0; i < capacity; ++i)
    klsymtbl_bucket_free(array[i]);
  free(array);
  map->array = NULL;
  map->capacity = 0;
  map->size = 0;
}

KlSymTbl* klsymtbl_create(size_t capacity, KlStrTab* strtab) {
  KlSymTbl* symtbl = (KlSymTbl*)malloc(sizeof (KlSymTbl));
  if (kl_unlikely(!symtbl || !klsymtbl_init(symtbl, capacity, strtab))) {
    free(strtab);
  }
  return symtbl;
}

void klsymtbl_delete(KlSymTbl* symtbl) {
  klsymtbl_destroy(symtbl);
  free(symtbl);
}

KlSymbol* klsymtbl_insert(KlSymTbl* symtbl, KlStrDesc name) {
  if (kl_unlikely(symtbl->size >= symtbl->capacity && !klsymtbl_expand(symtbl)))
    return NULL;

  KlSymbol* newsymbol = (KlSymbol*)malloc(sizeof (*newsymbol));
  if (kl_unlikely(!newsymbol)) return false;

  size_t hash = klsymtbl_hashing(symtbl->strtab, name);
  size_t index = (symtbl->capacity - 1) & hash;
  newsymbol->name = name;
  newsymbol->hash = hash;
  newsymbol->next = symtbl->array[index];
  symtbl->array[index] = newsymbol;
  symtbl->size++;
  return true;
}

KlSymbol* klsymtbl_search(KlSymTbl* map, KlStrDesc name) {
  size_t hash = klsymtbl_hashing(map->strtab, name);
  size_t index = (map->capacity - 1) & hash;
  KlSymbol* symbol = map->array[index];
  for (; symbol; symbol = symbol->next) {
    if (hash == symbol->hash && symbol->name.length == name.length &&
        strncmp(klstrtab_getstring(map->strtab, symbol->name.id),
                klstrtab_getstring(map->strtab, name.id), name.length)) {
      return symbol;
    }
  }

  return NULL;
}
