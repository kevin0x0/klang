#ifndef KEVCC_KLANG_INCLUDE_VALUE_VALUE_H
#define KEVCC_KLANG_INCLUDE_VALUE_VALUE_H
#include "klang/include/value/typedecl.h"
#include "utils/include/kstring/kstring.h"

#include <stdint.h>

typedef enum tagKlType { KL_INT, KL_MAP, KL_ARRAY, KL_STRING, KL_NIL } KlType;

typedef int64_t KlInt;

typedef struct tagKlValue {
  union {
    KlInt intval;
    KString* string;
    KlArray* array;
    KlMap* map;
  } value;
  KlType type;
  size_t ref_count;
  /* link to next value. used by memory manager module and garbage collector
   * module to link all allocated values. */
  struct tagKlValue* next; 
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

KlValue* klvalue_get_nil(KlValue* value);

static inline void klvalue_ref_incr(KlValue* value) {
  ++value->ref_count;
}
static inline void klvalue_ref_decr(KlValue* value) {
  --value->ref_count;
}


#endif
