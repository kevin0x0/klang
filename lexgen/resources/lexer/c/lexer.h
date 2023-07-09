#ifndef __KEVCC_LEXGEN_LEXER_H
#define __KEVCC_LEXGEN_LEXER_H

#include <stdint.h>
#include <stdbool.h>

typedef struct tagLex {
  uint8_t* buffer;
  size_t curpos;  /* current position */
} Lex;

typedef union tagTokenAttr {
  int64_t ival;
  double fval;
  char* sval;
} TokenAttr;

typedef struct tagToken {
  size_t begin;
  size_t end;
  int kind;
} Token;

bool lex_init(Lex* lex, char* filepath);
void lex_destroy(Lex* lex);
void lex_next(Lex* lex, Token* token);

#endif
