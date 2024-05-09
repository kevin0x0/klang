#ifndef __INCLUDE_ERROR_KLERROR_H__
#define __INCLUDE_ERROR_KLERROR_H__

#include "deps/k/include/kio/ki.h"
#include "deps/k/include/kio/ko.h"
#include <stddef.h>

typedef struct tagKlErrorConfig {
  unsigned tabstop;
  unsigned maxtextline;
  unsigned maxtextcol;
  char curl;
  char zerocurl;
  char* promptmsg;
  char* prompttext;
  char* promptnorm;
} KlErrorConfig;

typedef struct tagKlError {
  KlErrorConfig config;
  Ko* err;
  unsigned errcount;
} KlError;


typedef unsigned KlFileOffset;

void klerror_init(KlError* klerr, Ko* errout);
void klerror_error(KlError* klerr, Ki* input, const char* inputname, KlFileOffset begin, KlFileOffset end, const char* format, ...);
void klerror_errorv(KlError* klerr, Ki* input, const char* inputname, KlFileOffset begin, KlFileOffset end, const char* format, va_list args);

static inline unsigned klerror_nerror(KlError* klerr) {
  return klerr->errcount;
}

#endif
