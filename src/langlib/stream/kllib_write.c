#include "include/klapi.h"
#include "deps/k/include/kio/ko.h"
#include "include/langlib/stream/kllib_write.h"
#include "include/langlib/stream/kllib_stream.h"

#define KLLIB_BASIC_PRINT_DEPTH_LIMIT   (3)

static void kllib_ostream_write_map(KlState* state, Ko* ko, const KlMap* map, size_t depth);
static void kllib_ostream_write_tuple(KlState* state, Ko* ko, const KlTuple* tuple, size_t depth);
static void kllib_ostream_write_array(KlState* state, Ko* ko, const KlArray* array, size_t depth);
static void kllib_ostream_write_inner(KlState* state, Ko* ko, const KlValue* val, size_t depth);

static void kllib_ostream_write_inner(KlState* state, Ko* ko, const KlValue* val, size_t depth) {
  switch (klvalue_gettype(val)) {
    case KL_INT: {
      ko_printf(ko, "%lld", klvalue_getint(val));
      break;
    }
    case KL_FLOAT: {
      ko_printf(ko, "%lf", klvalue_getfloat(val));
      break;
    }
    case KL_STRING: {
      ko_putc(ko, '"');
      ko_write(ko, klstring_content(klvalue_getobj(val, KlString*)),
                   klstring_length(klvalue_getobj(val, KlString*)));
      ko_putc(ko, '"');
      break;
    }
    case KL_NIL: {
      ko_write(ko, "nil", sizeof ("nil") - 1);
      break;
    }
    case KL_BOOL: {
      ko_puts(ko, klvalue_getbool(val) == KL_TRUE ? "true" : "false");
      break;
    }
    case KL_CFUNCTION: {
      ko_printf(ko, "<%s: %p>", klexec_typename_cstr(state, val), klvalue_getgcobj(val));
      break;
    }
    case KL_TUPLE: {
      kllib_ostream_write_tuple(state, ko, klvalue_getobj(val, KlTuple*), depth + 1);
      break;
    }
    case KL_ARRAY: {
      kllib_ostream_write_array(state, ko, klvalue_getobj(val, KlArray*), depth + 1);
      break;
    }
    case KL_MAP: {
      kllib_ostream_write_map(state, ko, klvalue_getobj(val, KlMap*), depth + 1);
      break;
    }
    default: {
      ko_printf(ko, "<%s: %p>", klexec_typename_cstr(state, val), klvalue_getgcobj(val));
      break;
    }
  }
}

static void kllib_ostream_write_tuple(KlState* state, Ko* ko, const KlTuple* tuple, size_t depth) {
  const KlValue* end = kltuple_iter_end(tuple);
  const KlValue* itr = kltuple_iter_begin(tuple);
  if (itr == end) {
    ko_puts(ko, "()");
    return;
  }
  if (depth++ >= KLLIB_BASIC_PRINT_DEPTH_LIMIT) {
    ko_puts(ko, "(...)");
    return;
  }
  ko_putc(ko, '(');
  kllib_ostream_write_inner(state, ko, itr, depth);
  ++itr;
  for (; itr != end; ++itr) {
    ko_puts(ko, ", ");
    kllib_ostream_write_inner(state, ko, itr, depth);
  }
  ko_putc(ko, ')');
}

static void kllib_ostream_write_array(KlState* state, Ko* ko, const KlArray* array, size_t depth) {
  KlArrayIter end = klarray_iter_end(array);
  KlArrayIter itr = klarray_iter_begin(array);
  if (itr == end) {
    ko_puts(ko, "[]");
    return;
  }
  if (depth++ >= KLLIB_BASIC_PRINT_DEPTH_LIMIT) {
    ko_puts(ko, "[...]");
    return;
  }
  ko_putc(ko, '[');
  kllib_ostream_write_inner(state, ko, itr, depth);
  itr = klarray_iter_next(itr);
  for (; itr != end; itr = klarray_iter_next(itr)) {
    ko_puts(ko, ", ");
    kllib_ostream_write_inner(state, ko, itr, depth);
  }
  ko_putc(ko, ']');
}

static void kllib_ostream_write_map(KlState* state, Ko* ko, const KlMap* map, size_t depth) {
  size_t end = klmap_iter_end(map);
  size_t itr = klmap_iter_begin(map);
  if (itr == end) {
    ko_puts(ko, "{:}");
    return;
  }
  if (depth++ >= KLLIB_BASIC_PRINT_DEPTH_LIMIT) {
    ko_puts(ko, "{...}");
    return;
  }
  ko_putc(ko, '{');
  kllib_ostream_write_inner(state, ko, klmap_iter_getkey(map, itr), depth);
  ko_putc(ko, ':');
  kllib_ostream_write_inner(state, ko, klmap_iter_getvalue(map, itr), depth);
  itr = klmap_iter_next(map, itr);
  for (; itr != end; itr = klmap_iter_next(map, itr)) {
    ko_puts(ko, ", ");
    kllib_ostream_write_inner(state, ko, klmap_iter_getkey(map, itr), depth);
    ko_putc(ko, ':');
    kllib_ostream_write_inner(state, ko, klmap_iter_getvalue(map, itr), depth);
  }
  ko_putc(ko, '}');
}

KlException kllib_ostream_write(KlState* state) {
  size_t narg = klapi_narg(state);
  if (kl_unlikely(narg == 0 || !kllib_ostream_compatible(klapi_accessb(state, 0))))
    return klapi_throw_internal(state, KL_E_ARGNO, "please call with ostream object");
  KlOutputStream* ostream = klapi_getobjb(state, 0, KlOutputStream*);
  Ko* ko = kllib_ostream_getko(ostream);
  if (kl_unlikely(!ko))
    return klapi_throw_internal(state, KL_E_INVLD, "uninitialized ostream object");
  for (size_t i = 1; i < narg; ++i) {
    switch (klapi_gettypeb(state, i)) {
      case KL_INT: {
        ko_printf(ko, "%lld", klapi_getintb(state, i));
        break;
      }
      case KL_FLOAT: {
        ko_printf(ko, "%lf", klapi_getfloatb(state, i));
        break;
      }
      case KL_STRING: {
        ko_write(ko, klstring_content(klapi_getstringb(state, i)), klstring_length(klapi_getstringb(state, i)));
        break;
      }
      case KL_TUPLE: {
        kllib_ostream_write_tuple(state, ko, klapi_gettupleb(state, i), 0);
        break;
      }
      case KL_ARRAY: {
        kllib_ostream_write_array(state, ko, klapi_getarrayb(state, i), 0);
        break;
      }
      case KL_MAP: {
        kllib_ostream_write_map(state, ko, klapi_getmapb(state, i), 0);
        break;
      }
      case KL_NIL: {
        ko_puts(ko, "nil");
        break;
      }
      case KL_BOOL: {
        ko_puts(ko, klapi_getboolb(state, i) ? "true" : "false");
        break;
      }
      case KL_CFUNCTION: {
        ko_printf(ko, "<%s: %p>", klexec_typename_cstr(state, klapi_accessb(state, i)), klapi_getcfuncb(state, i));
        break;
      }
      default: {
        ko_printf(ko, "<%s: %p>", klexec_typename_cstr(state, klapi_accessb(state, i)), klapi_getgcobjb(state, i));
        break;
      }
    }
  }
  if (!kllib_ostream_testoption(ostream, KLLIB_OSTREAM_FBUF))
    ko_flush(ko);
  klapi_pop(state, narg - 1);
  return klapi_return(state, 1);
}

