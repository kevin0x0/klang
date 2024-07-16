#include "include/langlib/stream/kllib_kostring.h"
#include "include/kio/ko.h"
#include "include/langlib/stream/kllib_strbuf.h"
#include "include/misc/klutils.h"
#include "include/mm/klmm.h"
#include <stdlib.h>
#include <string.h>

typedef struct tagKoString {
  Ko ko;
  KlStringBuf strbuf;
} KoString;

static void kostring_delete(KoString* kostring);
static KioFileOffset kostring_size(KoString* kostring);
static void kostring_writer(KoString* kostring);

static const KoVirtualFunc kostring_vfunc = { .writer = (KoWriter)kostring_writer, .delete = (KoDelete)kostring_delete, .size = (KoSize)kostring_size };


Ko* kostring_create(size_t size) {
  KoString* kostring = (KoString*)malloc(sizeof (KoString));
  if (!kostring) return NULL;
  if (kl_unlikely(!klstrbuf_init(&kostring->strbuf, size))) {
    free(kostring);
    return NULL;
  }
  ko_init((Ko*)kostring, &kostring_vfunc);
  ko_setbuf((Ko*)kostring, klstrbuf_access(&kostring->strbuf, 0), klstrbuf_capacity(&kostring->strbuf), 0);
  return (Ko*)kostring;
}

static void kostring_delete(KoString* kostring) {
  klstrbuf_destroy(&kostring->strbuf);
  free(kostring);
}

static KioFileOffset kostring_size(KoString* kostring) {
  return klstrbuf_size(&kostring->strbuf);
}

static void kostring_writer(KoString* kostring) {
  size_t writeend = ko_tell((Ko*)kostring);
  if (writeend >= klstrbuf_capacity(&kostring->strbuf)) {
    if (kl_unlikely(!klstrbuf_recap(&kostring->strbuf, writeend + writeend / 2))) {
      ko_setbuf((Ko*)kostring, ko_getbuf((Ko*)kostring), 0, writeend);
      return;
    }
  }
  ko_setbuf((Ko*)kostring, klstrbuf_access(&kostring->strbuf, 0), klstrbuf_capacity(&kostring->strbuf), 0);
  ko_setbufcurr((Ko*)kostring, writeend);
  klstrbuf_setsize(&kostring->strbuf, writeend);
}

bool kostring_compatible(Ko* ko) {
  return ko_compatible(ko, &kostring_vfunc);
}

KlStringBuf* kostring_getstrbuf(KoString* ko) {
  return &ko->strbuf;
}
