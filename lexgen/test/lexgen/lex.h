
#ifndef __KEVCC_LEXGEN_LEXER_H
#define __KEVCC_LEXGEN_LEXER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define LEX_DEAD    (255)

#define TK_INT (0)
#define TK_NUM (1)
#define TK_ID (2)
#define TK_END (3)
#define TK_BLANK (4)



typedef union tagTokenAttr {
  int64_t ival;
  double fval;
  char* sval;
} TokenAttr;



typedef struct tagToken {
  size_t begin;
  size_t end;
  int kind;
  TokenAttr attr;
} Token;


typedef uint8_t (*TransTab)[256];
typedef void Callback(Token*, uint8_t*);

typedef struct tagLex {
  uint8_t* buffer;
  TransTab table;
  int* patterns;
  size_t start_state;
  Callback** callbacks;
} Lex;


bool lex_init(Lex* lex, char* filepath);
void lex_destroy(Lex* lex);
void lex_next(Lex* lex, Token* token);
const char* lex_get_info(Lex* lex, int kind);

static inline void lex_token_init(Token* token) {
  token->end = 0;
}

#endif

