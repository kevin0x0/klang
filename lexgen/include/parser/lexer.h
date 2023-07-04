#ifndef KEVCC_LEXGEN_INCLUDE_PARSER_LEXER_H
#define KEVCC_LEXGEN_INCLUDE_PARSER_LEXER_H

#include "lexgen/include/general/global_def.h"
#include <stdio.h>

#define KEV_LEXGEN_TOKEN_ERR          (-1)
#define KEV_LEXGEN_TOKEN_DEF          (0)
#define KEV_LEXGEN_TOKEN_NAME         (1)
#define KEV_LEXGEN_TOKEN_REGEX        (2)
#define KEV_LEXGEN_TOKEN_ASSIGN       (3)
#define KEV_LEXGEN_TOKEN_COLON        (4)
#define KEV_LEXGEN_TOKEN_BLANKS       (5)
#define KEV_LEXGEN_TOKEN_OPEN_PAREN   (6)
#define KEV_LEXGEN_TOKEN_CLOSE_PAREN  (7)
#define KEV_LEXGEN_TOKEN_END          (8)

typedef struct tagKevLexGenLexer {
  FILE* infile;
  size_t position;
  uint8_t (*table)[256];
  int* acc_mapping;
  size_t start;
} KevLexGenLexer;

typedef struct tagKevLexGenToken {
  size_t begin;
  size_t end;
  char* attr;
  int kind;
} KevLexGenToken;

bool kev_lexgenlexer_init(KevLexGenLexer* lex, const char* filepath);
void kev_lexgenlexer_destroy(KevLexGenLexer* lex);
 
bool kev_lexgenlexer_next(KevLexGenLexer* lex, KevLexGenToken* token);


#endif
