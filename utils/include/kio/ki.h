#ifndef KEVCC_UTILS_INCLUDE_KIO_KI_H
#define KEVCC_UTILS_INCLUDE_KIO_KI_H

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

#define KOF   EOF

typedef struct tagKi Ki;

typedef void (*KiReader)(Ki* ki);
typedef void (*KiDelete)(Ki* ki);
typedef size_t (*KiSize)(Ki* ki);


typedef struct KiVirtualFunc {
  KiReader reader;
  KiDelete delete;
  KiSize size;
} KiVirtualFunc;

struct tagKi {
  KiVirtualFunc* vfunc;
  const unsigned char* buf;
  const unsigned char* curr;
  const unsigned char* end;
  size_t headpos;
};

static inline void ki_init(Ki* ki, KiVirtualFunc* vfunc);
static inline void ki_delete(Ki* ki);

static inline size_t ki_bufused(Ki* ki);
static inline size_t ki_bufsize(Ki* ki);
static inline size_t ki_size(Ki* ki);
static inline size_t ki_tell(Ki* ki);
static inline void ki_seek(Ki* ki, size_t pos);

static inline int ki_getc(Ki* ki);
static inline void ki_ungetc(Ki* ki);
size_t ki_read(Ki* ki, void* buf, size_t bufsize);
int ki_loadnext(Ki* ki);


static inline void* ki_getbuf(Ki* ki);
static inline void ki_setbuf(Ki* ki, const void* buf, size_t size, size_t headpos);
static inline void ki_setbufcurr(Ki* ki, size_t offset);


static inline void ki_init(Ki* ki, KiVirtualFunc* vfunc) {
  ki->vfunc = vfunc;
  ki->buf = NULL;
  ki->curr = NULL;
  ki->end = NULL;
  ki->headpos = 0;
}

static inline void ki_delete(Ki* ki) {
  ki->vfunc->delete(ki);
}

static inline size_t ki_bufused(Ki* ki) {
  return ki->curr - ki->buf;
}

static inline size_t ki_bufsize(Ki* ki) {
  return ki->end - ki->buf;
}

static inline size_t ki_size(Ki* ki) {
  return ki->vfunc->size(ki);
}

static inline size_t ki_tell(Ki* ki) {
  return ki->headpos + (ki->curr - ki->buf);
}

static inline void ki_seek(Ki* ki, size_t pos) {
  if (pos >= ki->headpos && pos < ki->headpos + ki_bufsize(ki)) {
    ki->curr = ki->buf + pos - ki->headpos;
    return;
  }
  ki->curr = ki->buf;
  ki->end = ki->buf;
  ki->headpos = pos;
}

static inline int ki_getc(Ki* ki) {
  return ki->curr != ki->end ? *ki->curr++ : ki_loadnext(ki);
}

static inline void ki_ungetc(Ki* ki) {
  if (ki->curr == ki->buf)
    ki_seek(ki, ki_tell(ki) - 1);
  --ki->curr;
}


static inline void* ki_getbuf(Ki* ki) {
  return (void*)ki->buf;
}

static inline void ki_setbuf(Ki* ki, const void* buf, size_t size, size_t headpos) {
  ki->buf = buf;
  ki->curr = buf;
  ki->end = (unsigned char*)buf + size;
  ki->headpos = headpos;
}

static inline void ki_setbufcurr(Ki* ki, size_t offset) {
  ki->curr = ki->buf + offset;
}


#endif
