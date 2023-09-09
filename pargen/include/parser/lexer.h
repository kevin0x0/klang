#ifndef KEVCC_PARGEN_INCLUDE_PARSER_LEXER_H
#define KEVCC_PARGEN_INCLUDE_PARSER_LEXER_H

#include "utils/include/general/global_def.h"
#include "pargen/include/parser/partokens.h"

#include <stdio.h>

#define KEV_PLEX_BUF_SIZE   (4096)
#define KEV_PLEX_DEAD       (255)
#define KEV_PLEX_NONACC     (-1)


struct tagKevPLexer;
typedef void Callback(struct tagKevPLexer* lex);

typedef union tagKevPTokenAttr {
  int num;
  char* str;
} KevPTokenAttr;

typedef struct tagKevPToken {
  int kind;
  size_t begin;
  size_t end;
  KevPTokenAttr attr;
} KevPToken;

typedef struct tagKevPLexer {
  FILE* infile;
  size_t currpos; /* current position */
  KevPToken currtoken;
  uint8_t* buf;
  uint8_t (*table)[256];
  int* pattern_mapping;
  uint8_t start;
  Callback** callbacks;
  size_t err_count;
} KevPLexer;

bool kev_pargenlexer_init(KevPLexer* lex,FILE* infile);
void kev_pargenlexer_destroy(KevPLexer* lex);
 
void kev_pargenlexer_next(KevPLexer* lex);
const char* kev_pargenlexer_info(int kind);

uint8_t (*kev_lexgen_get_transition_table(void))[256];
int* kev_lexgen_get_pattern_mapping(void);
size_t kev_lexgen_get_start_state(void);
Callback** kev_lexgen_get_callbacks(void);
const char** kev_lexgen_get_info(void);

#endif
