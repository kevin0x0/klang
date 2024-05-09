#include "include/error/klerror.h"
#include "deps/k/include/kio/ki.h"
#include "deps/k/include/kio/ko.h"
#include "include/kio/kio_common.h"
#include <stdbool.h>

void klerror_init(KlError* klerr, Ko* errout) {
  klerr->err = errout;
  klerr->errcount = 0;
  klerr->config = (KlErrorConfig) {
    .curl = '~',
    .zerocurl = '^',
    .tabstop = 8,
    .maxtextline = 3,
    .promptnorm = "|| ",
    .prompttext = "||== ",
    .promptmsg = "|| "
  };
}


static unsigned klerror_helper_locateline(Ki* input, KlFileOffset offset);
static bool klerror_helper_showline_withcurl(KlError* klerr, Ki* input, KlFileOffset begin, KlFileOffset end);

void klerror_error(KlError* klerr, Ki* input, const char* inputname, KlFileOffset begin, KlFileOffset end, const char* format, ...) {
  va_list args;
  va_start(args, format);
  klerror_errorv(klerr, input, inputname, begin, end, format, args);
  va_end(args);
}

void klerror_errorv(KlError* klerr, Ki* input, const char* inputname, KlFileOffset begin, KlFileOffset end, const char* format, va_list args) {
  ++klerr->errcount;
  Ko* err = klerr->err;
  KioFileOffset orioffset = ki_tell(input);
  unsigned line = klerror_helper_locateline(input, begin);
  KioFileOffset linebegin = ki_tell(input);

  unsigned col = begin - linebegin + 1;
  ko_printf(err, "%s%s:%4u:%4u: ", klerr->config.promptmsg, inputname, line, col);
  ko_vprintf(err, format, args);
  ko_putc(err, '\n');

  unsigned nputline = 0;
  while (nputline++ < klerr->config.maxtextline) {
    if (!klerror_helper_showline_withcurl(klerr, input, begin, end)) {
      break;
    }
  }
  if (nputline > klerr->config.maxtextline)
    ko_printf(err, "%stoo many lines...\n", klerr->config.promptnorm);
  ki_seek(input, orioffset);
  ko_printf(err, "%s\n", klerr->config.promptnorm);
  //ko_putc(err, '\n');
  ko_flush(err);
}

#define kl_isnl(ch)       ((ch) == '\n' || (ch) == '\r')

static unsigned klerror_helper_locateline(Ki* input, KlFileOffset offset) {
  ki_seek(input, 0);
  unsigned currline = 1;
  KioFileOffset lineoff = 0;
  while (ki_tell(input) < offset) {
    int ch = ki_getc(input);
    if (ch == KOF) break;
    if (kl_isnl(ch)) {
      if ((ch = ki_getc(input)) != '\r' && ch != KOF)
        ki_ungetc(input);
      ++currline;
      lineoff = ki_tell(input);
    }
  }
  ki_seek(input, lineoff);
  return currline;
}

static bool klerror_helper_showline_withcurl(KlError* klerr, Ki* input, KlFileOffset begin, KlFileOffset end) {
  Ko* err = klerr->err;
  KioFileOffset curroffset = ki_tell(input);
  if (curroffset >= end) return false;
  ko_printf(err, "%s", klerr->config.prompttext);
  int ch = ki_getc(input);
  while (!kl_isnl(ch) && ch != KOF) {
    ko_putc(err, ch);
    ch = ki_getc(input);
  }
  ko_putc(err, '\n');
  ko_printf(err, "%s", klerr->config.prompttext);
  ki_seek(input, curroffset);
  ch = ki_getc(input);
  /* leading white space should not be underlined. */
  while (ch == ' ' || ch == '\t') {
    if (ch == '\t') {
      for (size_t i = 0; i < klerr->config.tabstop; ++i)
        ko_putc(err, ' ');
    } else {
      ko_putc(err, ' ');
    }
    ch = ki_getc(input);
    ++curroffset;
  }

  while (!kl_isnl(ch)) {
    if (curroffset == begin && curroffset == end) {
      ko_putc(err, klerr->config.zerocurl);
    } else if (curroffset >= begin && curroffset < end) {
      if (ch == '\t') {
        for (size_t i = 0; i < klerr->config.tabstop; ++i)
          ko_putc(err, klerr->config.curl);
      } else {
        ko_putc(err, klerr->config.curl);
      }
    } else {
      if (ch == '\t') {
        for (size_t i = 0; i < klerr->config.tabstop; ++i)
          ko_putc(err, ' ');
      } else {
        ko_putc(err, ' ');
      }
    }
    if (ch == KOF) break;
    ch = ki_getc(input);
    ++curroffset;
  }
  ko_putc(err, '\n');
  if (ch != KOF && (ch = ki_getc(input)) != '\r' && ch != KOF)
    ki_ungetc(input);
  return true;
}
