#ifndef __KEVCC_LEXGEN_LEXER_H
#define __KEVCC_LEXGEN_LEXER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define LEX_DEAD    (255)

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

typedef uint8_t (*TransTab)[256];
typedef void Callback(Token*, uint8_t*, uint8_t*);

typedef struct tagLex {
  uint8_t* buffer;
  uint8_t* curpos;  /* current position */
  TransTab table;
  int* patterns;
  size_t start_state;
  Callback** callbacks;
} Lex;


bool lex_init(Lex* lex, char* filepath);
void lex_destroy(Lex* lex);
void lex_next(Lex* lex, Token* token);

#endif