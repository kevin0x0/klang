#include "pargen/include/parser/lexer.h"
#include "utils/include/string/kev_string.h"

#include <stdlib.h>

bool kev_pargenlexer_init(KevPLexer* lex,FILE* infile, const char* filename) {
  if (!lex) return false;
  lex->err_count = 0;
  lex->currpos = 0;
  lex->infile = NULL;
  lex->buf = (uint8_t*)malloc(sizeof (uint8_t) * KEV_PLEX_BUF_SIZE);
  if (!lex->buf) return false;
  if (!(lex->filename = kev_str_copy(filename))) {
    free(lex->buf);
    lex->buf = NULL;
    return false;
  }
  lex->infile = infile;
  lex->currtoken.begin = 0;
  lex->currtoken.end = 0;
  lex->table = kev_lexgen_get_transition_table();
  lex->pattern_mapping = kev_lexgen_get_pattern_mapping();
  lex->start = kev_lexgen_get_start_state();
  lex->callbacks = kev_lexgen_get_callbacks();
  return true;
}

void kev_pargenlexer_destroy(KevPLexer* lex) {
  if (!lex) return;
  free(lex->buf);
  free(lex->filename);
  lex->err_count = 0;
  lex->buf = NULL;
  lex->infile = NULL;
  lex->currtoken.begin = 0;
  lex->currtoken.end = 0;
  lex->currpos = 0;
}
 
void kev_pargenlexer_next(KevPLexer* lex) {
  size_t tokenlen = 0;
  uint8_t (*table)[256] = lex->table;
  uint8_t state = lex->start;
  uint8_t next_state = 0;
  FILE* infile = lex->infile;
  uint8_t ch = (uint8_t)fgetc(infile);
  uint8_t* buf = lex->buf;
  while ((next_state = table[state][ch]) != KEV_PLEX_DEAD && tokenlen < KEV_PLEX_BUF_SIZE) {
    state = next_state;
    buf[tokenlen++] = ch;
    ch = (uint8_t)fgetc(infile);
  }
  ungetc(ch, infile);
  KevPToken* token = &lex->currtoken;
  token->begin = lex->currpos;
  token->end = lex->currpos + tokenlen;
  lex->currpos += tokenlen;
  token->kind = lex->pattern_mapping[state];
  if (lex->callbacks[state]) {
    lex->callbacks[state](lex);
  }
}

const char* kev_pargenlexer_info(int kind) {
  return kind == KEV_PTK_ERR ? "error" : kev_lexgen_get_info()[kind];
}

void kev_pargenlexer_free_attr(KevPLexer* lex) {
  if (lex->currtoken.kind == KEV_PTK_ID || lex->currtoken.kind == KEV_PTK_STR)
    free(lex->currtoken.attr.str);
}
