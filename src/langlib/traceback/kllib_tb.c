#include "include/klapi.h"
#include "include/lang/klconfig.h"

#include <stdio.h>

typedef struct tagKlFmtConfig {
  unsigned tabstop;
  unsigned maxtextline;
  unsigned maxframe;
  char curl;
  char zerocurl;
  char* promptmsg;
  char* prompttext;
  char* promptnorm;
} KlFmtConfig;

static unsigned kllib_tb_helper_locateline(FILE* input, long offset);
static bool kllib_tb_helper_showline_withcurl(KlFmtConfig* fmt, FILE* err, FILE* input, long begin, long end);

static void kllib_tb_printCframe(KlFmtConfig* fmt, FILE* err);
static void kllib_tb_printKframe_noany(KlFmtConfig* fmt, FILE* err);
static void kllib_tb_printKframe_noposinfo(KlFmtConfig* fmt, FILE* err, const char* inputname);
static void kllib_tb_printKframe_nosrcfile(KlFmtConfig* fmt, FILE* err, long begin, long end);
static void kllib_tb_printKframe_openfilefailure(KlFmtConfig* fmt, FILE* err, const char* inputname, long begin, long end);
static void kllib_tb_printKframe(KlFmtConfig* fmt, FILE* err, FILE* input, const char* inputname, long begin, long end);

static void kllib_tb_printframe(KlFmtConfig* fmt, FILE* err, const KlCallInfo* callinfo);
static void kllib_tb_dotraceback(KlFmtConfig* fmt, FILE* err, const KlCallInfo* from, const KlCallInfo* end);
static KlException kllib_tb_main(KlState* state);

KlException KLCONFIG_LIBRARY_TRACEBACK_ENTRYFUNCNAME(KlState* state) {
  KLAPI_PROTECT(klapi_checkstack(state, 1));
  klapi_pushcfunc(state, kllib_tb_main);
  return klapi_return(state, 1);
}

static KlException kllib_tb_main(KlState* state) {
  KlCallInfo* currci = klstate_currci(state);
  kl_assert(currci->status & KLSTATE_CI_STATUS_CFUN && currci->callable.cfunc == kllib_tb_main, "");
  FILE* err = NULL;
  if (klapi_narg(state) == 0) {
    err = stderr;
  } else {
    KlValue* firstarg = klapi_accessb(state, 0);
    kl_assert(klvalue_checktype(firstarg, KL_USERDATA),  "expected file pointer(use type tag: KL_USERDATA)");
    err = klcast(FILE*, klvalue_getuserdata(firstarg));
  }
  KlFmtConfig fmtconfig = {
    .curl = '~',
    .zerocurl = '^',
    .tabstop = 8,
    .maxtextline = 3,
    .maxframe = 10,
    .promptnorm = "|| ",
    .prompttext = "||== ",
    .promptmsg = "|| ",
  };
  /* the actual most recent callinfo is this function itself, so use currci->prev
   * instead of currci. */
  kllib_tb_dotraceback(&fmtconfig, err, currci->prev, klstate_baseci(state));
  return klapi_return(state, 0);
}

static void kllib_tb_dotraceback(KlFmtConfig* fmt, FILE* err, const KlCallInfo* from, const KlCallInfo* end) {
  unsigned count = 0;
  while (from != end && count++ < fmt->maxframe) {
    kllib_tb_printframe(fmt, err, from);
    from = from->prev;
  }

  if (from != end)
    fprintf(err, "%stoo many frames...\n", fmt->promptmsg);
}

static void kllib_tb_printframe(KlFmtConfig* fmt, FILE* err, const KlCallInfo* callinfo) {
  if (!(callinfo->status & KLSTATE_CI_STATUS_KCLO)) {
    kllib_tb_printCframe(fmt, err);
    return;
  }
  KlKFunction* kfunc = klcast(KlKClosure*, callinfo->callable.clo)->kfunc;
  KlKFuncFilePosition* posinfo = klkfunc_posinfo(kfunc);
  const KlString* srcfile = klkfunc_srcfile(kfunc);
  if (!posinfo && !srcfile) {
    kllib_tb_printKframe_noany(fmt, err);
    return;
  }
  if (!posinfo) {
    kllib_tb_printKframe_noposinfo(fmt, err, klstring_content(srcfile));
    return;
  }
  if (!srcfile) {
    KlKFuncFilePosition pos = posinfo[(callinfo->savedpc - klkfunc_entrypoint(kfunc)) - 1];
    kllib_tb_printKframe_nosrcfile(fmt, err, pos.begin, pos.end);
    return;
  }
  FILE* input = fopen(klstring_content(srcfile), "rb");
  if (kl_unlikely(!input)) {
    KlKFuncFilePosition pos = posinfo[(callinfo->savedpc - klkfunc_entrypoint(kfunc)) - 1];
    kllib_tb_printKframe_openfilefailure(fmt, err, klstring_content(srcfile), pos.begin, pos.end);
    return;
  }
  KlKFuncFilePosition pos = posinfo[(callinfo->savedpc - klkfunc_entrypoint(kfunc)) - 1];
  kllib_tb_printKframe(fmt, err, input, klstring_content(srcfile), pos.begin, pos.end);
  fclose(input);
  return;
}

static void kllib_tb_printCframe(KlFmtConfig* fmt, FILE* err) {
  fprintf(err, "%s[C] %s:%4s:%4s:\n", fmt->promptmsg, "??", "??", "??");
  fprintf(err, "%s\n", fmt->promptnorm);
}

static void kllib_tb_printKframe_noany(KlFmtConfig* fmt, FILE* err) {
  fprintf(err, "%s[K] no backtrace infomation\n", fmt->promptmsg);
  fprintf(err, "%s\n", fmt->promptnorm);
}

static void kllib_tb_printKframe_noposinfo(KlFmtConfig* fmt, FILE* err, const char* inputname) {
  fprintf(err, "%s[K] %s: offset = (begin = %s, end = %s)\n", fmt->promptmsg, inputname, "unknown", "unknown");
  fprintf(err, "%s\n", fmt->promptnorm);
}

static void kllib_tb_printKframe_nosrcfile(KlFmtConfig* fmt, FILE* err, long begin, long end) {
  fprintf(err, "%s[K] %s: offset = (begin = %ld, end = %ld)\n", fmt->promptmsg, "unknown", begin, end);
  fprintf(err, "%s\n", fmt->promptnorm);
}

static void kllib_tb_printKframe_openfilefailure(KlFmtConfig* fmt, FILE* err, const char* inputname, long begin, long end) {
  fprintf(err, "%s[K] %s: offset = (begin = %ld, end = %ld)\n", fmt->promptmsg,inputname, begin, end);
  fprintf(err, "%sfailed to open source file.\n", fmt->promptmsg);
}

static void kllib_tb_printKframe(KlFmtConfig* fmt, FILE* err, FILE* input, const char* inputname, long begin, long end) {
  long orioffset = ftell(input);
  unsigned line = kllib_tb_helper_locateline(input, begin);
  long linebegin = ftell(input);

  unsigned col = begin - linebegin + 1;
  fprintf(err, "%s[K] %s:%4u:%4u:\n", fmt->promptmsg, inputname, line, col);

  unsigned nputline = 0;
  while (nputline++ < fmt->maxtextline) {
    if (!kllib_tb_helper_showline_withcurl(fmt, err, input, begin, end)) {
      break;
    }
  }
  if (nputline > fmt->maxtextline)
    fprintf(err, "%stoo many lines...\n", fmt->promptnorm);
  fseek(input, orioffset, SEEK_SET);
  fprintf(err, "%s\n", fmt->promptnorm);
}

#define kl_isnl(ch)       ((ch) == '\n' || (ch) == '\r')

static unsigned kllib_tb_helper_locateline(FILE* input, long offset) {
  fseek(input, 0, SEEK_SET);
  unsigned currline = 1;
  long curroff = 0;
  long lineoff = 0;
  while (curroff < offset) {
    int ch = fgetc(input);
    ++curroff;
    if (ch == EOF) break;
    if (kl_isnl(ch)) {
      ++currline;
      if (ch == '\r') {
        if ((ch = fgetc(input)) != '\n') {
          ungetc(ch, input);
        } else {
          ++curroff;
        }
      }
      lineoff = curroff;
      if (lineoff < 0) {
        lineoff = 0;
        return currline;
      }
    }
  }
  fseek(input, lineoff, SEEK_SET);
  return currline;
}

static bool kllib_tb_helper_showline_withcurl(KlFmtConfig* fmt, FILE* err, FILE* input, long begin, long end) {
  long curroffset = ftell(input);
  if (curroffset >= end) return false;
  fprintf(err, "%s", fmt->prompttext);
  int ch = fgetc(input);
  while (!kl_isnl(ch) && ch != EOF) {
    if (ch == '\t') {
      for (size_t i = 0; i < fmt->tabstop; ++i)
        fputc(' ', err);
    } else {
      fputc(ch, err);
    }
    ch = fgetc(input);
  }
  fputc('\n', err);
  fprintf(err, "%s", fmt->prompttext);
  fseek(input, curroffset, SEEK_SET);
  ch = fgetc(input);
  /* leading white space should not be underlined. */
  while (ch == ' ' || ch == '\t') {
    if (ch == '\t') {
      for (size_t i = 0; i < fmt->tabstop; ++i)
        fputc(' ', err);
    } else {
      fputc(' ', err);
    }
    ch = fgetc(input);
    ++curroffset;
  }

  while (!kl_isnl(ch)) {
    if (curroffset == begin && curroffset == end) {
      fputc(fmt->zerocurl, err);
    } else if (curroffset >= begin && curroffset < end) {
      if (ch == '\t') {
        for (size_t i = 0; i < fmt->tabstop; ++i)
          fputc(fmt->curl, err);
      } else {
        fputc(fmt->curl, err);
      }
    } else {
      if (ch == '\t') {
        for (size_t i = 0; i < fmt->tabstop; ++i)
          fputc(' ', err);
      } else {
        fputc(' ', err);
      }
    }
    if (ch == EOF) break;
    ch = fgetc(input);
    ++curroffset;
  }
  if (ch == '\r') {
    if ((ch = fgetc(input)) != '\n')
      ungetc(ch, input);
  }
  fputc('\n', err);
  return true;
}
