#ifndef KEVCC_KLANG_INCLUDE_PARSE_KLLEX_H
#define KEVCC_KLANG_INCLUDE_PARSE_KLLEX_H

#include "klang/include/parse/klstrtab.h"
#include "klang/include/parse/kltokens.h"
#include "klang/include/value/klvalue.h"
#include "utils/include/kio/kio.h"

#include <stdint.h>

#define KLLEX_STRLIMIT  (KLSTRTAB_EXTRA)


typedef struct tagKlFilePos {
  uint32_t line;
  uint32_t offset;
} KlFilePos;

typedef struct tagKlLex {
  KlStrTab* strtab;         /* string table */
  Ki* input;                /* input stream */
  char* inputname;          /* name of input stream */
  Ko* err;                  /* error information puts here. */
  size_t nerror;            /* number of lexical error */
  size_t currline;          /* current line number */
  struct {
    KlFilePos begin;        /* begin position of this token */
    KlFilePos end;          /* end position of this token */
    union {
      KlInt intval;
      KlBool boolval;
      KlStrDesc string;
    };
    KlTokenKind kind;
  } tok;                    /* token information */
} KlLex;

bool kllex_init(KlLex* lex, Ki* ki, Ko* err, const char* inputname, KlStrTab* strtab);
void kllex_destroy(KlLex* lex);
KlLex* kllex_create(Ki* ki, Ko* err, const char* inputname, KlStrTab* strtab);
void kllex_delete(KlLex* lex);

void kllex_next(KlLex* lex);
static inline KlTokenKind kllex_tokkind(KlLex* lex);
static inline bool kllex_check(KlLex* lex, KlTokenKind kind);


void kllex_error(KlLex* lex, const char* format, ...);
void kllex_show_info(KlLex* lex, const char* format, va_list vlst);

static inline KlTokenKind kllex_tokkind(KlLex* lex) {
  return lex->tok.kind;
}

static inline bool kllex_check(KlLex* lex, KlTokenKind kind) {
  return kllex_tokkind(lex) == kind;
}

#endif
