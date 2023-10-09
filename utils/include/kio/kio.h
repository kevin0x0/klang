#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

typedef const void* KioBufReader(void* data, size_t next_readpos, size_t readpos, const void* buf, size_t* p_buf_size);
typedef void* KioBufWriter(void* data, size_t next_writepos, size_t writepos, void* buf, size_t* p_buf_size);

typedef struct tagKi {
  const char* buf;
  const char* buf_end;
  const char* readpos;
  size_t headpos;
  void* data;
  KioBufReader* reader;
} Ki;

typedef struct tagKo {
  char* buf;
  char* buf_end;
  char* writepos;
  size_t headpos;
  void* data;
  KioBufWriter* writer;
} Ko;

typedef struct tagKio {
  Ki ki;
  Ko ko;
} Kio;

static inline void ki_init(Ki* ki, void* data, KioBufReader* reader);
static inline void ki_destroy(Ki* ki);
static inline size_t ki_bufsize(Ki* ki);
static inline size_t ki_tell(Ki* ki);
bool ki_seek(Ki* ki, size_t offset);
int ki_fill_buf(Ki* ki);

static inline int ki_getc(Ki* ki);
size_t ki_read(Ki* ki, void* buf, size_t buf_size);


static inline void ko_init(Ko* ko, void* data, KioBufWriter* writer);
static inline void ko_destroy(Ko* ko);
static inline size_t ko_bufsize(Ko* ko);
static inline size_t ko_tell(Ko* ko);
bool ko_seek(Ko* ko, size_t offset);
bool ko_write_buf(Ko* ko, int ch);

static inline bool ko_putc(Ko* ko, int ch);
size_t ko_write(Ko* ko, void* buf, size_t buf_size);


bool ki_init_file(Ki* ki, FILE* file, size_t bufsize);
bool ki_init_open(Ki* ki, const char* filepath, size_t bufsize);
bool ko_init_file(Ko* ko, FILE* file, size_t bufsize);
bool ko_init_open(Ko* ko, const char* filepath, size_t bufsize);
bool ki_init_buf(Ki* ki, const void* buf, size_t bufsize);
bool ko_init_buf(Ko* ko, void* buf, size_t bufsize);
void ki_init_string(Ki* ki, const char* str);





static inline void ki_init(Ki* ki, void* data, KioBufReader* reader) {
  if (!ki || !reader) return;
  ki->buf = NULL;
  ki->buf_end = NULL;
  ki->readpos = NULL;
  ki->headpos = 0;
  ki->data = data;
  ki->reader = reader;
}

static inline void ki_destroy(Ki* ki) {
  if (!ki) return;
  if (ki->buf) {
    size_t buf_size = ki_bufsize(ki);
    ki->reader(ki->data, ki->headpos, ki->headpos, ki->buf, &buf_size);
  }
  ki->buf = NULL;
  ki->buf_end = NULL;
  ki->readpos = NULL;
  ki->data = NULL;
  ki->headpos = 0;
  ki->reader = NULL;
}

static inline size_t ki_bufsize(Ki* ki) {
  return ki->buf_end - ki->buf;
}

static inline size_t ki_tell(Ki* ki) {
  return (ki->readpos - ki->buf) + ki->headpos;
}

static inline int ki_getc(Ki* ki) {
  return ki->readpos != ki->buf_end ? (int)*ki->readpos++ : ki_fill_buf(ki);
}


static inline void ko_init(Ko* ko, void* data, KioBufWriter* writer) {
  if (!ko || !writer) return;
  ko->buf = NULL;
  ko->buf_end = NULL;
  ko->writepos = NULL;
  ko->headpos = 0;
  ko->data = data;
  ko->writer = writer;
}

static inline void ko_destroy(Ko* ko) {
  if (!ko) return;
  if (ko->buf) {
    size_t writesize = ko->writepos - ko->buf;
    ko->writer(ko->data, ko->headpos, ko->headpos, ko->buf, &writesize);
  }
  ko->buf = NULL;
  ko->buf_end = NULL;
  ko->writepos = NULL;
  ko->data = NULL;
  ko->headpos = 0;
  ko->writepos = NULL;
}

static inline size_t ko_bufsize(Ko* ko) {
  return ko->buf_end - ko->buf;
}

static inline size_t ko_tell(Ko* ko) {
  return (ko->writepos - ko->buf) + ko->headpos;
}

static inline bool ko_putc(Ko* ko, int ch) {
  return ko->writepos == ko->buf_end ? ko_write_buf(ko, ch) : (*ko->writepos++ = ch, true);
}
