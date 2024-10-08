#ifndef _KLANG_INCLUDE_ERROR_KLERROR_H_
#define _KLANG_INCLUDE_ERROR_KLERROR_H_

#include "deps/k/include/kio/ki.h"
#include "deps/k/include/kio/ko.h"

typedef struct tagKlErrorConfig {
  unsigned tabstop;
  unsigned maxtextline;
  unsigned maxtextcol;
  unsigned maxreport;
  char* promptmsg;
  char* prompttext;
  char* promptnorm;
  char curl;
  char zerocurl;
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
