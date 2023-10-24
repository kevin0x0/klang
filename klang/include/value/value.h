#ifndef KEVCC_KLANG_INCLUDE_VALUE_VALUE_H
#define KEVCC_KLANG_INCLUDE_VALUE_VALUE_H
#include "klang/include/value/typedecl.h"
#include "utils/include/kstring/kstring.h"

#include <stdint.h>

typedef enum tagKlType { KL_INT, KL_MAP, KL_ARRAY, KL_STRING, KL_NIL } KlType;
typedef enum tagKlGCStat { KL_GC_UNMARKED, KL_GC_VISIT, KL_GC_REACHABLE } KlGCStat;


typedef int64_t KlInt;

typedef struct tagKlValue {
  union {
    KlInt intval;
    KString* string;
    KlArray* array;
    KlMap* map;
  } value;
  /* link to next value. used by memory manager module and garbage collector
   * module to link all allocated values. */
  struct tagKlValue* next; 
  KlType type;
  /* unsed by garbage collector */
  KlGCStat gc_stat;
  /* unsed by garbage collector, link all reachable object */
  struct tagKlValue* next_reachable;
} KlValue;

KlValue* klvalue_create_array(void);
KlValue* klvalue_create_array_move(KlArray* array);
KlValue* klvalue_create_map(void);
KlValue* klvalue_create_map_move(KlMap* map);
KlValue* klvalue_create_string(const KString* string);
KlValue* klvalue_create_string_move(KString* string);
KlValue* klvalue_create_int(KlInt val);
KlValue* klvalue_create_shallow_copy(KlValue* value);
void klvalue_delete(KlValue* value);

static inline void klvalue_ref_incr(KlValue* value);
static inline void klvalue_ref_decr(KlValue* value);
static inline size_t klvalue_ref_count(KlValue* value);
static inline KlType klvalue_get_type(KlValue* value);

KlValue* klvalue_get_nil(void);

static inline KlType klvalue_get_type(KlValue* value) {
  return value->type;
}

#endif
