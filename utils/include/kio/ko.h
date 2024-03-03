#ifndef KEVCC_UTILS_INCLUDE_KIO_KO_H
#define KEVCC_UTILS_INCLUDE_KIO_KO_H

#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>

typedef struct tagKo Ko;

typedef void (*KoWriter)(Ko* ko);
typedef void (*KoDelete)(Ko* ko);
typedef size_t (*KoSize)(Ko* ko);


typedef struct KoVirtualFunc {
  KoWriter writer;
  KoDelete delete;
  KoSize size;
} KoVirtualFunc;

struct tagKo {
  KoVirtualFunc* vfunc;
  unsigned char* buf;
  unsigned char* curr;
  unsigned char* end;
  size_t headpos;
};


static inline void ko_init(Ko* ko, KoVirtualFunc* vfunc);
static inline void ko_delete(Ko* ko);

static inline size_t ko_bufused(Ko* ko);
static inline size_t ko_bufsize(Ko* ko);
static inline size_t ko_size(Ko* ko);
static inline size_t ko_tell(Ko* ko);
static inline void ko_seek(Ko* ko, size_t pos);

static inline void ko_putc(Ko* ko, int ch);
size_t ko_write(Ko* ko, void* buf, size_t bufsize);
void ko_flush(Ko* ko);
void ko_writenext(Ko* ko, int ch);

int ko_printf(Ko* ko, const char* fmt, ...);
int ko_vprintf(Ko* ko, const char* fmt, va_list arglist);


static inline void* ko_getbuf(Ko* ko);
static inline void ko_setbuf(Ko* ko, void* buf, size_t size, size_t headpos);
static inline void ko_setbufcurr(Ko* ko, size_t offset);


static inline void ko_init(Ko* ko, KoVirtualFunc* vfunc) {
  ko->vfunc = vfunc;
  ko->buf = NULL;
  ko->curr = NULL;
  ko->end = NULL;
  ko->headpos = 0;
}

static inline void ko_delete(Ko* ko) {
  if (ko_bufused(ko) != 0)
    ko->vfunc->writer(ko);
  ko->vfunc->delete(ko);
}

static inline size_t ko_bufused(Ko* ko) {
  return ko->curr - ko->buf;
}

static inline size_t ko_bufsize(Ko* ko) {
  return ko->end - ko->buf;
}

static inline size_t ko_size(Ko* ko) {
  return ko->vfunc->size(ko);
}

static inline size_t ko_tell(Ko* ko) {
  return ko->headpos + (ko->curr - ko->buf);
}

static inline void ko_seek(Ko* ko, size_t pos) {
  if (ko->curr != ko->buf)
    ko->vfunc->writer(ko);

  ko->curr = ko->buf;
  ko->end = ko->buf;
  ko->headpos = pos;
}

static inline void ko_putc(Ko* ko, int ch) {
  if (ko->curr != ko->end) {
    *ko->curr++ = (char)ch;
  } else {
    ko_writenext(ko, ch);
  }
}

static inline void* ko_getbuf(Ko* ko) {
  return ko->buf;
}

static inline void ko_setbuf(Ko* ko, void* buf, size_t size, size_t headpos) {
  ko->buf = buf;
  ko->curr = buf;
  ko->end = (unsigned char*)buf + size;
  ko->headpos = headpos;
}

static inline void ko_setbufcurr(Ko* ko, size_t offset) {
  ko->curr = ko->buf + offset;
}

#endif
