#ifndef KEVCC_LEXGEN_INCLUDE_PARSER_LEXER_H
#define KEVCC_LEXGEN_INCLUDE_PARSER_LEXER_H

#include "utils/include/general/global_def.h"

#include "lexgen/include/parser/lextokens.h"

#include <stdio.h>

#define KEV_LTK_ERR          (-1)

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
const char* kev_lexgenlexer_info(int kind);

uint8_t (*kev_lexgen_get_transition_table(void))[256];
int* kev_lexgen_get_pattern_mapping(void);
size_t kev_lexgen_get_start_state(void);
const char** kev_lexgen_get_info(void);

#endif
