#include "include/value/klstring.h"
#include "include/common/kltypes.h"
#include "include/misc/klutils.h"
#include "include/mm/klmm.h"
#include <string.h>


static KlGCObject* propagate(KlStrPool* strpool, KlMM* klmm, KlGCObject* gclist);
static void delete(KlStrPool* strpool, KlMM* klmm);
static void post(KlStrPool* strpool, KlMM* klmm);

static const KlGCVirtualFunc gcvfunc = { .destructor = (KlGCDestructor)delete, .propagate = (KlGCProp)propagate, .after = (KlGCAfter)post };


static KlString* search(KlStrPool* strpool, const char* str, size_t hash);
static KlString* insert(KlStrPool* strpool, KlString* klstr);

static KlString* create_concat(KlMM* klmm, const char* str1, size_t len1, const char* str2, size_t len2);

static KlString* create_buf(KlMM* klmm, const char* buf, size_t buflen) {
  if (kl_unlikely(buflen > KLUINT_MAX)) return NULL;
  KlString* klstr = (KlString*)klmm_alloc(klmm, sizeof (KlString) + buflen + 1);
  if (kl_unlikely(!klstr)) return NULL;

  memcpy(klstr->content, buf, buflen);
  klstr->content[buflen] = '\0';
  klstr->length = buflen;
  return klstr;
}

static KlString* create_concat(KlMM* klmm, const char* str1, size_t len1, const char* str2, size_t len2) {
  if (kl_unlikely(len1 + len2 > KLUINT_MAX)) return NULL;
  KlString* klstr = (KlString*)klmm_alloc(klmm, sizeof (KlString) + len1 + len2 + 1);
  if (kl_unlikely(!klstr)) return NULL;

  memcpy(klstr->content, str1, len1);
  memcpy(klstr->content + len1, str2, len2 + 1);
  klstr->length = len1 + len2;
  return klstr;
}

static inline size_t fastpower(size_t base, size_t exp) {
  size_t res = 1;
  while (exp != 0) {
    if (exp & 0x1) res *= base;
    base *= base;
    exp >>= 1;
  }
  return res;
}

static inline size_t hash_combine(const KlString* str1, const KlString* str2) {
  return klstring_hash(str1) * (fastpower(65599, klstring_length(str2))) + klstring_hash(str2);
}

static inline size_t hash(const char* str) {
  size_t hash = 0;
  while (*str)
    hash = (*str++) + (hash << 6) + (hash << 16) - hash;
  return hash;
}

static inline size_t calculate_hash_buf(const char* str, const char* end) {
  size_t hash = 0;
  while (str != end)
    hash = (*str++) + (hash << 6) + (hash << 16) - hash;
  return hash;
}



static void rehash(KlStrPool* strpool, KlString** new_array, size_t new_capacity) {
  size_t mask = new_capacity - 1;
  KlString** bucketend = strpool->array + strpool->capacity;
  for (KlString** bucket = strpool->array; bucket != bucketend; ++bucket) {
    KlString* str = *bucket;
    while (str) {
      KlString* next = str->next;
      size_t index = str->hash & mask;
      str->next = new_array[index];
      new_array[index] = str;
      str = next;
    }
  }
  klmm_free(klstrpool_getmm(strpool), strpool->array, strpool->capacity * sizeof (KlString*));
  strpool->array = new_array;
  strpool->capacity = new_capacity;
}

static inline bool expand(KlStrPool* strpool) {
  size_t new_capacity = strpool->capacity << 1;
  KlString** new_array = klmm_alloc(klstrpool_getmm(strpool), new_capacity * sizeof (KlString*));
  if (kl_unlikely(!new_array)) return false;
  for (size_t i = 0; i < new_capacity; ++i)
    new_array[i] = NULL;
  rehash(strpool, new_array, new_capacity);
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
  strpool->lstrings = NULL;
  strpool->capacity = capacity;
  strpool->size = 0;
  strpool->klmm = klmm;
  klmm_gcobj_enable(klmm, klmm_to_gcobj(strpool), &gcvfunc);
  return strpool;
}

static KlGCObject* propagate(KlStrPool* strpool, KlMM* klmm, KlGCObject* gclist) {
  klmm_gcobj_aftersweep(klmm, klmm_to_gcobj(strpool));
  return gclist;
}

static void delete(KlStrPool* strpool, KlMM* klmm) {
  KlString** bucketend = strpool->array + strpool->capacity;
  /* free short strings */
  for (KlString** bucket = strpool->array; bucket != bucketend; ++bucket) {
    KlString* str = *bucket;
    while (str) {
      KlString* next = str->next;
      klmm_free(klmm, str, klstring_size(str));
      str = next;
    }
  }
  /* free long strings */
  KlString* lstrings = strpool->lstrings;
  while (lstrings) {
    KlString* tmp = lstrings->next;
    klmm_free(klmm, lstrings, klstring_size(lstrings));
    lstrings = tmp;
  }
  klmm_free(klmm, strpool->array, strpool->capacity * sizeof (KlString*));
  klmm_free(klmm, strpool, sizeof (KlStrPool));
}

static KlString* search_buf(KlStrPool* strpool, const char* buf, size_t buflen, size_t hash) {
  size_t mask = strpool->capacity - 1;
  size_t index = hash & mask;
  KlString* node = strpool->array[index];
  while (node) {
    if (hash == node->hash &&
        strncmp(buf, node->content, buflen) == 0)
      return node;
    node = node->next;
  }
  return NULL;
}

static KlString* search(KlStrPool* strpool, const char* str, size_t hash) {
  size_t mask = strpool->capacity - 1;
  size_t index = hash & mask;
  KlString* node = strpool->array[index];
  while (node) {
    if (hash == node->hash &&
        strcmp(str, node->content) == 0)
      return node;
    node = node->next;
  }
  return NULL;
}

static KlString* insert(KlStrPool* strpool, KlString* str) {
  if (kl_unlikely(klstring_islong(str))) {
    str->next = strpool->lstrings;
    strpool->lstrings = str;
    klmm_gcobj_enable_delegate(klstrpool_getmm(strpool), klmm_to_gcobjdelegate(str));
    return str;
  }

  if (kl_unlikely(strpool->size >= strpool->capacity && !expand(strpool)))
    return NULL;

  size_t index = (strpool->capacity - 1) & klstring_hash(str);
  str->next = strpool->array[index];
  strpool->array[index] = str;
  strpool->size++;
  klmm_gcobj_enable_delegate(klstrpool_getmm(strpool), klmm_to_gcobjdelegate(str));
  return str;
}

KlString* klstrpool_new_string_buf(KlStrPool* strpool, const char* buf, size_t buflen) {
  size_t hash = calculate_hash_buf(buf, buf + buflen);
  KlString* res = search_buf(strpool, buf, buflen, hash);
  if (res) return res;
  KlMM* klmm = klstrpool_getmm(strpool);
  KlString* klstr = create_buf(klmm, buf, buflen);
  if (!klstr) return NULL;
  klstr->hash = hash;
  return insert(strpool, klstr);
}

KlString* klstrpool_string_concat_cstyle(KlStrPool* strpool, const char* str1, const char* str2) {
  KlMM* klmm = klstrpool_getmm(strpool);
  KlString* klstr = create_concat(klmm, str1, strlen(str1), str2, strlen(str2));
  if (kl_unlikely(!klstr)) return NULL;
  const char* conc = klstring_content(klstr);
  size_t hashval = hash(conc);
  if (kl_likely(!klstring_islong(klstr))) {
    KlString* res = search(strpool, conc, hashval);
    if (res) {
      klmm_free(klmm, klstr, klstring_size(klstr));
      return res;
    }
  }
  klstr->hash = hashval;
  return insert(strpool, klstr);
}

KlString* klstrpool_string_concat(KlStrPool* strpool, const KlString* str1, const KlString* str2) {
  KlMM* klmm = klstrpool_getmm(strpool);
  KlString* klstr = create_concat(klmm, klstring_content(str1), str1->length, klstring_content(str2), str2->length);
  if (kl_unlikely(!klstr)) return NULL;
  const char* conc = klstring_content(klstr);
  size_t hashval = hash_combine(str1, str2);
  if (kl_likely(!klstring_islong(klstr))) {
    KlString* res = search(strpool, conc, hashval);
    if (res) {
      klmm_free(klmm, klstr, klstring_size(klstr));
      return res;
    }
  }
  klstr->hash = hashval;
  return insert(strpool, klstr);
}

static void post(KlStrPool* strpool, KlMM* klmm) {
  KlString** bucketend = strpool->array + strpool->capacity;
  size_t freecount = 0;
  /* handle short strings */
  for (KlString** bucket = strpool->array; bucket != bucketend; ++bucket) {
    KlString** pstr = bucket;
    KlString* str;
    while ((str = *pstr)) {
      if (klmm_gcobj_isalive(str)) {
        klmm_gcobj_clearalive(str);
        pstr = &str->next;
      } else {
        *pstr = str->next;
        klmm_free(klmm, str, klstring_size(str));
        ++freecount;
      }
    }
  }
  strpool->size -= freecount;
  /* handle long strings */
  KlString* survivors = NULL;
  KlString* lstrings = strpool->lstrings;
  while (lstrings) {
    KlString* tmp = lstrings->next;
    if (klmm_gcobj_isalive(lstrings)) {
      klmm_gcobj_clearalive(lstrings);
      lstrings->next = survivors;
      survivors = lstrings;
    } else {
      klmm_free(klmm, lstrings, klstring_size(lstrings));
    }
    lstrings = tmp;
  }
}
