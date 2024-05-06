#ifndef _KLANG_INCLUDE_PARSE_KLLEX_H_
#define _KLANG_INCLUDE_PARSE_KLLEX_H_

#include "include/ast/klast.h"
#include "include/ast/klstrtbl.h"
#include "include/parse/kltokens.h"
#include "include/error/klerror.h"
#include "deps/k/include/kio/ki.h"

#include <stdint.h>

#define KLLEX_STRLIMIT  (KLSTRTAB_EXTRA)

typedef struct tagKlLex {
  KlStrTbl* strtbl;         /* string table */
  Ki* input;                /* input stream */
  char* inputname;          /* name of input stream */
  size_t nerror;            /* number of lexical error */
  size_t currline;          /* current line number */
  KlError* klerror;         /* error reporter */
  struct {
    KlFileOffset begin;     /* begin position of this token */
    KlFileOffset end;       /* end position of this token */
    KlTokenKind kind;
    union {
      KlCInt intval;
      KlCFloat floatval;
      KlCBool boolval;
      KlStrDesc string;
    };
    bool hasleadingblank;   /* whether there is blank before this token */
  } tok;                    /* token information */
} KlLex;

bool kllex_init(KlLex* lex, Ki* ki, KlError* klerr, const char* inputname, KlStrTbl* strtbl);
void kllex_destroy(KlLex* lex);
KlLex* kllex_create(Ki* ki, KlError* klerr, const char* inputname, KlStrTbl* strtbl);
void kllex_delete(KlLex* lex);

void kllex_next(KlLex* lex);
static inline bool kllex_hasleadingblank(KlLex* lex);
static inline bool kllex_trymatch(KlLex* lex, KlTokenKind kind);
static inline bool kllex_check(KlLex* lex, KlTokenKind kind);
static inline KlTokenKind kllex_tokkind(KlLex* lex);
static inline Ki* kllex_inputstream(KlLex* lex);


void kllex_error(KlLex* lex, const char* format, ...);
void kllex_show_info(KlLex* lex, const char* format, va_list vlst);

static inline bool kllex_hasleadingblank(KlLex* lex) {
  return lex->tok.hasleadingblank;
}

static inline bool kllex_trymatch(KlLex* lex, KlTokenKind kind) {
  if (lex->tok.kind == kind) {
    kllex_next(lex);
    return true;
  }
  return false;
}

static inline bool kllex_check(KlLex* lex, KlTokenKind kind) {
  return kllex_tokkind(lex) == kind;
}

static inline KlTokenKind kllex_tokkind(KlLex* lex) {
  return lex->tok.kind;
}

static inline Ki* kllex_inputstream(KlLex* lex) {
  return lex->input;
}

#endif
