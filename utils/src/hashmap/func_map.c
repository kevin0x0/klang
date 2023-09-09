#include "utils/include/hashmap/func_map.h"


KevFuncObject* kev_funcmap_insert(KevFuncMap* funcs, const char* funcname, void* func, void* object) {
  KevFuncObject* funcobj = (KevFuncObject*)malloc(sizeof (KevFuncObject));
  if (!funcobj) return NULL;
  funcobj->func = func;
  funcobj->object = object;
  if (!kev_strxmap_insert(funcs, funcname, funcobj)) {
    free(funcobj);
    return NULL;
  }
  return funcobj;
}

KevFuncObject* kev_funcmap_search(KevFuncMap* funcs, const char* funcname) {
  KevStrXMapNode* node = kev_strxmap_search(funcs, funcname);
  return node ? (KevFuncObject*)node->value : NULL;

}
