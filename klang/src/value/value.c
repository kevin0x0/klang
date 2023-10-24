#include "klang/include/value/value.h"
#include "klang/include/value/klarray.h"
#include "klang/include/value/klmap.h"
#include "klang/include/value/typedecl.h"
#include "klang/include/objpool/value_pool.h"
#include "utils/include/kstring/kstring.h"

#include <stdlib.h>

KlValue kl_nil_struct = { { 0 }, NULL, KL_NIL, false , NULL };

static KlValue* kl_nil = &kl_nil_struct;

KlValue* klvalue_create_array(void) {
  KlArray* array = klarray_create();
  KlValue* value = klvalue_pool_allocate();
  if (!array || !value) {
    klarray_delete(array);
    klvalue_pool_deallocate(value);
    return NULL;
  }
  value->value.array = array;
  value->type = KL_ARRAY;
  return value;
}

KlValue* klvalue_create_array_move(KlArray* array) {
  KlValue* value = klvalue_pool_allocate();
  if (!array || !value) {
    klvalue_pool_deallocate(value);
    return NULL;
  }
  value->value.array = array;
  value->type = KL_ARRAY;
  return value;
}

KlValue* klvalue_create_map(void) {
  KlValue* value = klvalue_pool_allocate();
  KlMap* map = klmap_create(4);
  if (!map || !value) {
    klvalue_pool_deallocate(value);
    return NULL;
  }
  value->value.map = map;
  value->type = KL_MAP;
  return value;
}

KlValue* klvalue_create_map_move(KlMap* map) {
  KlValue* value = klvalue_pool_allocate();
  if (!map || !value) {
    klvalue_pool_deallocate(value);
    return NULL;
  }
  value->value.map = map;
  value->type = KL_MAP;
  return value;
}

KlValue* klvalue_create_string(const KString* string) {
  KString* copy = kstring_create_copy(string);
  KlValue* value = klvalue_pool_allocate();
  if (!copy || !value) {
    kstring_delete(copy);
    klvalue_pool_deallocate(value);
    return NULL;
  }
  value->value.string = copy;
  value->type = KL_STRING;
  return value;
}

KlValue* klvalue_create_string_move(KString* string) {
  KlValue* value = klvalue_pool_allocate();
  if (!string || !value) {
    klvalue_pool_deallocate(value);
    return NULL;
  }
  value->value.string = string;
  value->type = KL_STRING;
  return value;
}

KlValue* klvalue_create_int(KlInt val) {
  KlValue* value = klvalue_pool_allocate();
  if (!value) {
    klvalue_pool_deallocate(value);
    return NULL;
  }
  value->value.intval = val;
  value->type = KL_INT;
  return value;
}

KlValue* klvalue_create_shallow_copy(KlValue* value) {
  KlValue* copy = klvalue_pool_allocate();
  if (!copy) {
    klvalue_pool_deallocate(value);
    return NULL;
  }
  switch (value->type) {
    case KL_INT:
      copy->value.intval = value->value.intval;
      break;
    case KL_STRING:
      copy->value.string = kstring_create_copy(value->value.string);
      break;
    case KL_MAP:
      copy->value.map = klmap_create_copy(value->value.map);
      break;
    case KL_ARRAY:
      copy->value.array = klarray_create_copy(value->value.array);
      break;
    default:
      /* impossible state */
      exit(EXIT_FAILURE);
  }
  copy->type = value->type;
  return copy;
}

void klvalue_delete(KlValue* value) {
  switch (value->type) {
    case KL_ARRAY:
      klarray_delete(value->value.array);
      break;
    case KL_MAP:
      klmap_delete(value->value.map);
      break;
    case KL_STRING:
      kstring_delete(value->value.string);
      break;
    case KL_NIL:
      return;
    case KL_INT:
    default:
      break;
  }
  klvalue_pool_deallocate(value);
}

KlValue* klvalue_get_nil(void) {
  return kl_nil;
}
