#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "lexgen/include/parser/lexer.h"

#include <stdio.h>

#define KEV_PLEXER_DEAD         ((uint8_t)255)
#define KEV_PLEXER_NONACC       (-1)

static char lex_attr_buffer[1024];

uint8_t (*kev_lexgenlexer_get_table(void))[256];
int* kev_lexgenlexer_get_acc_array(void);
size_t kev_lexgenlexer_get_start_state(void);

bool kev_lexgenlexer_init(KevLLexer* lex, FILE* infile) {
  if (!lex) return false;
  lex->infile = infile;
  lex->currpos = 0;
  lex->table = kev_lexgenlexer_get_table();
  lex->acc_mapping = kev_lexgenlexer_get_acc_array();
  lex->start = kev_lexgenlexer_get_start_state();
  return true;
}

void kev_lexgenlexer_destroy(KevLLexer* lex) {
  if (lex) {
    lex->infile = NULL;
    lex->currpos = 0;
    lex->acc_mapping = NULL;
    lex->table = NULL;
  }
}
 
bool kev_lexgenlexer_next(KevLLexer* lex, KevLToken* token) {
  size_t position = 0;
  uint8_t (*table)[256] = lex->table;
  uint8_t state = lex->start;
  uint8_t next_state = 0;
  FILE* infile = lex->infile;
  uint8_t ch = (uint8_t)fgetc(infile);
  while ((next_state = table[state][ch]) != KEV_LEXGENLEXER_DEAD && position < sizeof (lex_attr_buffer) / sizeof (uint8_t)) {
    state = next_state;
    lex_attr_buffer[position++] = ch;
    ch = (uint8_t)fgetc(infile);
  }
  ungetc(ch, infile);
  lex_attr_buffer[position] = '\0';
  token->begin = lex->currpos;
  token->end = lex->currpos + position;
  lex->currpos += position;
  if (lex->acc_mapping[state] == KEV_LEXGENLEXER_NONACC) {
    token->kind = KEV_LTK_ERR;
    if (position == 0)
      lex->currpos++;
    fgetc(infile);
    return false;
  }
  token->attr = lex_attr_buffer;
  token->kind = lex->acc_mapping[state];
  return true;
}

char* kev_lexgenlexer_info(int kind) {
  return token_info[kind];
}
