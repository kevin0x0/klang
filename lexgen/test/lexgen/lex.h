
#ifndef __KEVCC_LEXGEN_LEXER_H
#define __KEVCC_LEXGEN_LEXER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define LEX_DEAD    (255)

#define TK_WHILE (0)
#define TK_IF (1)
#define TK_ELSE (2)
#define TK_FOR (3)
#define TK_DO (4)
#define TK_FN (5)
#define TK_LAMBDA (6)
#define TK_LET (7)
#define TK_I32 (8)
#define TK_I64 (9)
#define TK_I16 (10)
#define TK_U8 (11)
#define TK_U32 (12)
#define TK_U64 (13)
#define TK_U16 (14)
#define TK_I8 (15)
#define TK_LBC (16)
#define TK_RBC (17)
#define TK_LBK (18)
#define TK_RBK (19)
#define TK_OPEN (20)
#define TK_CLOSE (21)
#define TK_LAB (22)
#define TK_RAB (23)
#define TK_ADD (24)
#define TK_SUB (25)
#define TK_MUL (26)
#define TK_DIV (27)
#define TK_ASSIGN (28)
#define TK_COLON (29)
#define TK_SEMI (30)
#define TK_COMMA (31)
#define TK_DOT (32)
#define TK_BAR (33)
#define TK_DBAR (34)
#define TK_AND (35)
#define TK_DAND (36)
#define TK_EQUAL (37)
#define TK_NEQUAL (38)
#define TK_LE (39)
#define TK_GE (40)
#define TK_ID (41)
#define TK_NUM (42)
#define TK_FLT (43)
#define TK_STR (44)
#define TK_END (45)



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
} tokenizer;


bool lex_init(tokenizer* lex, char* filepath);
void lex_destroy(tokenizer* lex);
void lex_next(tokenizer* lex, Token* token);
const char* lex_get_info(tokenizer* lex, int kind);

static inline void lex_token_init(Token* token) {
  token->end = 0;
}

#endif

