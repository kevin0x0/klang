#ifndef KEVCC_UTILS_INCLUDE_HASHMAP_FUNC_MAP_H
#define KEVCC_UTILS_INCLUDE_HASHMAP_FUNC_MAP_H

#include "utils/include/hashmap/strx_map.h"
#include <stdlib.h>

typedef KevStrXMap KevFuncMap;
typedef KevStrXMapNode KevFuncMapNode;

typedef struct tagKevFuncObject {
  void* func;
  void* object;
} KevFuncObject;

static inline KevFuncMap* kev_funcmap_create(void);
static inline void kev_funcmap_delete(KevFuncMap* funcs);
KevFuncObject* kev_funcmap_insert(KevFuncMap* funcs, const char* funcname, void* func, void* object);
KevFuncObject* kev_funcmap_search(KevFuncMap* funcs, const char* funcname);




static inline KevFuncMap* kev_funcmap_create(void) {
  return kev_strxmap_create(8);
}

static inline void kev_funcmap_delete(KevFuncMap* funcs) {
  if (!funcs) return;
  KevStrXMapNode* node = kev_strxmap_iterate_begin(funcs);
  for (; node; node = kev_strxmap_iterate_next(funcs, node)) {
    free(node->value);
  }
  kev_strxmap_delete(funcs);
}

#endif
