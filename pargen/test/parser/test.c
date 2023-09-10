#include "pargen/include/parser/lexer.h"
#include "pargen/include/parser/parser.h"


int main(int argc, char** argv) {
  KevPParserState pstate;
  KevPLexer lex;
  FILE* infile = fopen("test.txt", "r");
  kev_pargenlexer_init(&lex, infile);
  kev_pargenparser_init(&pstate);
  kev_pargenparser_parse(&pstate, &lex);
  kev_pargenparser_destroy(&pstate);
  kev_pargenlexer_destroy(&lex);
  return 0;
}
