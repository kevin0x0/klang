#ifndef KEVCC_PARGEN_INCLUDE_PARSER_LEXER_H
#define KEVCC_PARGEN_INCLUDE_PARSER_LEXER_H

#include "utils/include/general/global_def.h"

#include <stdio.h>

#define KEV_PTK_DECL    (0)
#define KEV_PTK_ID      (1)
#define KEV_PTK_STR     (2)
#define KEV_PTK_LP      (3)
#define KEV_PTK_RP      (4)
#define KEV_PTK_LBE     (5)
#define KEV_PTK_RBE     (6)
#define KEV_PTK_LBC     (7)
#define KEV_PTK_RBC     (8)
#define KEV_PTK_COLON   (9)
#define KEV_PTK_SEMI    (10)
#define KEV_PTK_ASSIGN  (11)
#define KEV_PTK_ENV     (12)

typedef struct tagKevPLexer {
  FILE* infile;
  size_t currpos; /* current position */
} KevPLexer;

typedef struct tagKevPToken {
  int kind;
  size_t begin;
  size_t end;
  char* attr;
} KevPToken;

bool kev_pargenlexer_init(KevPLexer* lex,FILE* infile);
void kev_pargenlexer_destroy(KevPLexer* lex);
 
bool kev_pargenlexer_next(KevPLexer* lex, KevPToken* token);
char* kev_pargenlexer_info(int kind);

#endif
