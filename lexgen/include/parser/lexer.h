#ifndef KEVCC_LEXGEN_INCLUDE_PARSER_LEXER_H
#define KEVCC_LEXGEN_INCLUDE_PARSER_LEXER_H

#include "utils/include/general/global_def.h"

#include <stdio.h>

#define KEV_LTK_ERR          (-1)
#define KEV_LTK_DEF          (0)
#define KEV_LTK_IMPORT       (1)
#define KEV_LTK_ID           (2)
#define KEV_LTK_REGEX        (3)
#define KEV_LTK_ASSIGN       (4)
#define KEV_LTK_COLON        (5)
#define KEV_LTK_BLANKS       (6)
#define KEV_LTK_OPEN_PAREN   (7)
#define KEV_LTK_CLOSE_PAREN  (8)
#define KEV_LTK_ENV_VAR_DEF  (9)
#define KEV_LTK_END          (10)
#define KEV_LTK_LONG_STR     (11)
#define KEV_LTK_STR          (12)
#define KEV_LTK_NUMBER       (13)
#define KEV_LTK_COMMA        (14)

typedef struct tagKevLLexer {
  FILE* infile;
  size_t currpos; /* current position */
  uint8_t (*table)[256];
  int* acc_mapping;
  size_t start;
} KevLLexer;

typedef struct tagKevLToken {
  size_t begin;
  size_t end;
  char* attr;
  int kind;
} KevLToken;

bool kev_lexgenlexer_init(KevLLexer* lex,FILE* infile);
void kev_lexgenlexer_destroy(KevLLexer* lex);
 
bool kev_lexgenlexer_next(KevLLexer* lex, KevLToken* token);
char* kev_lexgenlexer_info(int kind);

#endif
