#ifndef KEVCC_KLANG_INCLUDE_PARSE_KLLEX_H
#define KEVCC_KLANG_INCLUDE_PARSE_KLLEX_H

#include "klang/include/parse/kltokens.h"
#include "klang/include/value/klvalue.h"
#include "utils/include/kio/kio.h"

#include <stdint.h>

#define KLLEX_BUFSIZE (4096)


typedef struct tagKlFilePos {
  uint32_t line;
  uint32_t offset;
} KlFilePos;

typedef struct tagKlLex {
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
      char* string;
      char* id;
    };
    KlToken kind;
  } tok;                    /* token information */
  char buf[KLLEX_BUFSIZE];
} KlLex;

bool kllex_init(KlLex* lex, Ki* ki, Ko* err, const char* inputname);
void kllex_destroy(KlLex* lex);
KlLex* kllex_create(Ki* ki, Ko* err, const char* inputname);
void kllex_delete(KlLex* lex);

void kllex_next(KlLex* lex);

void kllex_error(KlLex* lex, const char* format, ...);
void kllex_show_info(KlLex* lex, const char* format, va_list vlst);

#endif
