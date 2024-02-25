#include "utils/include/kio/kobuf.h"
#include <stdlib.h>

typedef struct tagKoBuf {
  Ko ko;
  void* buf;
  size_t bufsize;
} KoBuf;

void kobuf_delete(KoBuf* kobuf);
size_t kobuf_size(KoBuf* kobuf);
void kobuf_writer(KoBuf* kobuf);

static KoVirtualFunc kobuf_vfunc = { .writer = (KoWriter)kobuf_writer, .delete = (KoDelete)kobuf_delete, .size = (KoSize)kobuf_size };


Ko* kobuf_create(void* buf, size_t bufsize) {
  KoBuf* kobuf = (KoBuf*)malloc(sizeof (KoBuf));
  if (!kobuf) return NULL;
  kobuf->buf = buf;
  kobuf->bufsize = bufsize;
  ko_init((Ko*)kobuf, &kobuf_vfunc);
  ko_setbuf((Ko*)kobuf, buf, bufsize, 0);
  return (Ko*)kobuf;
}

void kobuf_delete(KoBuf* kobuf) {
  free(kobuf);
}

size_t kobuf_size(KoBuf* kobuf) {
  return kobuf->bufsize;
}

void kobuf_writer(KoBuf* kobuf) {
  size_t currpos = ko_tell((Ko*)kobuf);
  if (currpos >= kobuf->bufsize) {
    ko_setbuf((Ko*)kobuf, ko_getbuf((Ko*)kobuf), 0, currpos);
    return;
  }
  ko_setbuf((Ko*)kobuf, kobuf->buf, kobuf->bufsize, 0);
  ko_setbufcurr((Ko*)kobuf, currpos);
}
