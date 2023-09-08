#include "pargen/include/parser/lexer.h"

#include <stdlib.h>

#define KEV_PLEX_BUF_SIZE   (1024)

bool kev_pargenlexer_init(KevPLexer* lex,FILE* infile) {
  if (!lex) return false;
  lex->currpos = 0;
  lex->infile = NULL;
  lex->buf = (char*)malloc(sizeof (char) * KEV_PLEX_BUF_SIZE);
  if (!lex->buf) return false;
  lex->infile = infile;
  lex->currtoken.begin = 0;
  lex->currtoken.end = 0;
  return true;
}

void kev_pargenlexer_destroy(KevPLexer* lex) {
  if (!lex) return;
  free(lex->buf);
  free(lex);
}
 
bool kev_pargenlexer_next(KevPLexer* lex) {
  return false;
}

const char* kev_pargenlexer_info(int kind) {
  return kev_lexgen_get_info()[kind];
}

