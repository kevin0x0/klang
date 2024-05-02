#include "include/value/klstring.h"
#include <string.h>


static void klstrpool_string_on_delete(KlString* klstr, KlMM* klmm);
static void klstrpool_delete(KlStrPool* strpool, KlMM* klmm);

static KlGCVirtualFunc klstring_gcvfunc = { .destructor = (KlGCDestructor)klstrpool_string_on_delete, .propagate = NULL, .post = NULL };
static KlGCVirtualFunc klstrpool_gcvfunc = { .destructor = (KlGCDestructor)klstrpool_delete, .propagate = NULL, .post = NULL };

static inline void klstrpool_node_insert(KlString* insertpos, KlString* node);
static inline void klstrpool_init_head_tail(KlString* head, KlString* tail);

static KlString* klstrpool_search(KlStrPool* strpool, const char* str, size_t hash);
static KlString* klstrpool_insert(KlStrPool* strpool, KlString* klstr);
static void klstrpool_remove(KlString* klstr);

static KlString* klstring_create(KlMM* klmm, const char* str);
static KlString* klstring_create_concat(KlMM* klmm, const char* str1, const char* str2);

static KlString* klstring_create(KlMM* klmm, const char* str) {
  size_t len = strlen(str);
  KlString* klstr = (KlString*)klmm_alloc(klmm, sizeof (KlString) + len);
  if (!klstr) return NULL;

  memcpy(klstr->strhead, str, len + 1);
  klstr->length = len;
  return klstr;
}

static KlString* klstring_create_concat(KlMM* klmm, const char* str1, const char* str2) {
  size_t len1 = strlen(str1);
  size_t len2 = strlen(str2);
  KlString* klstr = (KlString*)klmm_alloc(klmm, sizeof (KlString) + len1 + len2);
  if (!klstr) return NULL;

  memcpy(klstr->strhead, str1, len1);
  memcpy(klstr->strhead + len1, str2, len2 + 1);
  klstr->length = len1 + len2;
  return klstr;
}


static inline void klstrpool_init_head_tail(KlString* head, KlString* tail) {
  head->next = tail;
  tail->prev = head;
  head->prev = NULL;
  tail->next = NULL;
}

static inline void klstrpool_node_insert(KlString* insertpos, KlString* node) {
  node->prev = insertpos->prev;
  insertpos->prev->next = node;
  node->next = insertpos;
  insertpos->prev = node;
}

static inline size_t klstring_calculate_hash(const char* str) {
  size_t hash = 0;
  size_t shift = 16;
  const char* p = str;
  while (*p != '\0') {
    hash += *p++ << shift;
    shift += 4;
    shift %= 32;
  }
  return hash;
}



static void klstrpool_rehash(KlStrPool* strpool, KlString** new_array, size_t new_capacity) {
  size_t mask = new_capacity - 1;
  KlString* str = strpool->head.next;
  KlString* end = &strpool->tail;

  KlString tmphead;
  KlString tmptail;
  klstrpool_init_head_tail(&tmphead, &tmptail);

  while (str != end) {
    KlString* tmp = str->next;
    size_t index = str->hash & mask;
    if (!new_array[index]) {
      /* this means 'node' is the first element that put in this bucket,
       * so this bucket has not added to bucket list yet. */
      new_array[index] = str;
      klstrpool_node_insert(tmphead.next, str);
    } else {
      klstrpool_node_insert(new_array[index]->next, str);
    }
    str = tmp;
  }

  tmphead.next->prev = &strpool->head;
  strpool->head.next = tmphead.next;
  tmptail.prev->next = &strpool->tail;
  strpool->tail.prev = tmptail.prev;

  klmm_free(klstrpool_getmm(strpool), strpool->array, strpool->capacity * sizeof (KlString*));
  strpool->array = new_array;
  strpool->capacity = new_capacity;
}

static inline bool klstrpool_expand(KlStrPool* strpool) {
  size_t new_capacity = strpool->capacity << 1;
  KlString** new_array = klmm_alloc(klstrpool_getmm(strpool), new_capacity * sizeof (KlString*));
  if (kl_unlikely(!new_array)) return false;
  for (size_t i = 0; i < new_capacity; ++i)
    new_array[i] = NULL;
  klstrpool_rehash(strpool, new_array, new_capacity);
  return true;
}

KlStrPool* klstrpool_create(KlMM* klmm, size_t capacity) {
  KlStrPool* strpool = (KlStrPool*)klmm_alloc(klmm, sizeof (KlStrPool));
  if (kl_unlikely(!strpool)) return NULL;

  strpool->array = (KlString**)klmm_alloc(klmm, capacity * sizeof (KlString*));
  if (kl_unlikely(!strpool->array)) {
    klmm_free(klmm, strpool, sizeof (KlStrPool));
    return NULL;
  }
  KlString** array = strpool->array;
  for (size_t i = 0; i < capacity; ++i)
    array[i] = NULL;
  strpool->capacity = capacity;
  strpool->size = 0;
  strpool->klmm = klmm;
  klstrpool_init_head_tail(&strpool->head, &strpool->tail);
  klmm_gcobj_enable(klmm, klmm_to_gcobj(strpool), &klstrpool_gcvfunc);
  return strpool;
}

static void klstrpool_delete(KlStrPool* strpool, KlMM* klmm) {
  KlString* str = strpool->head.next;
  KlString* end = &strpool->tail;
  while (str != end) {
    str->strpool = NULL;
    str = str->next;
  }
  klmm_free(klmm, strpool->array, strpool->capacity * sizeof (KlString*));
  klmm_free(klmm, strpool, sizeof (KlStrPool));
}

static KlString* klstrpool_search(KlStrPool* strpool, const char* str, size_t hash) {
  size_t mask = strpool->capacity - 1;
  size_t index = hash & mask;
  KlString* node = strpool->array[index];
  if (!node) return klstrpool_iter_end(strpool);
  do {
    if (hash == node->hash &&
        strcmp(str, node->strhead) == 0)
      return node;
    node = node->next;
  } while (node != &strpool->tail && (node->hash & mask) == index);
  return klstrpool_iter_end(strpool);
}

static KlString* klstrpool_insert(KlStrPool* strpool, KlString* str) {
  if (kl_unlikely(strpool->size >= strpool->capacity && !klstrpool_expand(strpool)))
    return NULL;

  size_t index = (strpool->capacity - 1) & klstring_hash(str);
  if (!strpool->array[index]) {
    strpool->array[index] = str;
    klstrpool_node_insert(strpool->head.next, str);
  } else {
    klstrpool_node_insert(strpool->array[index]->next, str);
  }
  strpool->size++;
  klmm_gcobj_enable(klstrpool_getmm(strpool), klmm_to_gcobj(str), &klstring_gcvfunc);
  return str;
}

KlString* klstrpool_new_string(KlStrPool* strpool, const char* str) {
  size_t hash = klstring_calculate_hash(str);
  KlString* res = klstrpool_search(strpool, str, hash);
  if (res != klstrpool_iter_end(strpool))
    return res;
  KlMM* klmm = klstrpool_getmm(strpool);
  KlString* klstr = klstring_create(klmm, str);
  if (!klstr) return NULL;
  klstr->hash = hash;
  klstr->strpool = strpool;
  return klstrpool_insert(strpool, klstr);
}

KlString* klstrpool_string_concat(KlStrPool* strpool, KlString* str1, KlString* str2) {
  KlMM* klmm = klstrpool_getmm(strpool);
  KlString* klstr = klstring_create_concat(klmm, klstring_content(str1), klstring_content(str2));
  if (!klstr) return NULL;
  const char* conc = klstring_content(klstr);
  size_t hash = klstring_calculate_hash(conc);
  KlString* res = klstrpool_search(strpool, conc, hash);
  if (res != klstrpool_iter_end(strpool)) {
    klmm_free(klmm, klstr, klstring_size(klstr));
    return res;
  }
  klstr->hash = hash;
  klstr->strpool = strpool;
  return klstrpool_insert(strpool, klstr);
}

static void klstrpool_string_on_delete(KlString* klstr, KlMM* klmm) {
  klstrpool_remove(klstr);
  klmm_free(klmm, klstr, klstring_size(klstr));
}

static void klstrpool_remove(KlString* klstr) {
  KlStrPool* strpool = klstr->strpool;
  if (kl_unlikely(!strpool)) return;
  klstr->prev->next = klstr->next;
  klstr->next->prev = klstr->prev;
  --strpool->size;
  size_t index = (strpool->capacity - 1) & klstr->hash;
  if (strpool->array[index] != klstr)
    return;
  KlString* next = klstr->next;
  if (next == &strpool->tail || (next->hash & (strpool->capacity - 1)) != index) {
    strpool->array[index] = NULL;
  } else {
    strpool->array[index] = next;
  }
}
