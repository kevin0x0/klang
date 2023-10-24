#include "klang/include/klgc.h"
#include "klang/include/value/klarray.h"
#include "klang/include/value/klmap.h"

#include <stdlib.h>

#define gclink(list, value) { value->next_reachable = list; list = value; }
#define gclist_pop(list) { list = list->next_reachable; }

#define listlink(list, value) { value->next = list; list = value; }

static inline KlValue* nextval(KlValue* value) {
  return value->next;
}

static KlValue* klgc_traverse_map(KlValue* gclist, KlValue* map);
static KlValue* klgc_traverse_array(KlValue* gclist, KlValue* map);


KlValue* klgc_start(KlValue* root, KlValue* valuelist) {
  KlValue* gclist = NULL;
  for (KlValue* ptr = valuelist; ptr; ptr = nextval(ptr)) {
      ptr->gc_stat = KL_GC_UNMARKED;
  }
  gclink(gclist, root);
  root->gc_stat = KL_GC_REACHABLE;
  return gclist;
}

void klgc_mark_reachable(KlValue* gclist) {
  while (gclist) {
    KlValue* val = gclist;
    gclist_pop(gclist);
    switch (klvalue_get_type(val)) {
      case KL_MAP:
        gclist = klgc_traverse_map(gclist, val);
        break;
      case KL_ARRAY:
        gclist = klgc_traverse_array(gclist, val);
        break;
      default:
        /* impossible state */
        exit(EXIT_FAILURE);
        break;
    }
  }
}

KlValue* klgc_clean(KlValue* valuelist, size_t* p_listlen) {
  KlValue* cleanedlist = NULL;
  size_t listlen = 0;
  for (KlValue* ptr = valuelist; ptr;) {
    KlValue* tmp = nextval(ptr);
    if (ptr->gc_stat == KL_GC_UNMARKED) {
      klvalue_delete(ptr);
    } else {
      listlink(cleanedlist, ptr);
      listlen++;
    }
    ptr = tmp;
  }
  *p_listlen = listlen;
  return cleanedlist;
}

static KlValue* klgc_traverse_map(KlValue* gclist, KlValue* value) {
  KlMapIter end = klmap_iter_end(value->value.map);
  KlMapIter begin = klmap_iter_begin(value->value.map);
  for (KlMapIter itr = begin; itr != end; itr = klmap_iter_next(itr)) {
    KlValue* val = itr->value;
    if (val->gc_stat != KL_GC_REACHABLE) {
      val->gc_stat = KL_GC_REACHABLE;
      if (klvalue_get_type(val) == KL_MAP ||
          klvalue_get_type(val) == KL_ARRAY) {
        gclink(gclist, val);
      }
    }
  }
  return gclist;
}

static KlValue* klgc_traverse_array(KlValue* gclist, KlValue* value) {
  KlArrayIter end = klarray_iter_end(value->value.array);
  KlArrayIter begin = klarray_iter_begin(value->value.array);
  for (KlArrayIter itr = begin; itr != end; itr = klarray_iter_next(itr)) {
    KlValue* val = *itr;
    if (val->gc_stat != KL_GC_REACHABLE) {
      val->gc_stat = KL_GC_REACHABLE;
      if (klvalue_get_type(val) == KL_MAP ||
          klvalue_get_type(val) == KL_ARRAY) {
        gclink(gclist, val);
      }
    }
  }
  return gclist;
}

void klgc_clean_all(KlValue* valuelist) {
  for (KlValue* ptr = valuelist; ptr;) {
    KlValue* tmp = nextval(ptr);
    klvalue_delete(ptr);
    ptr = tmp;
  }
}
