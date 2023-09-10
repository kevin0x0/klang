#include "pargen/include/parser/lexer.h"
#include "pargen/include/parser/partokens.h"

int main(int argc, char *argv[]) {
  KevPLexer lex;
  FILE* file = fopen("test.txt", "r");
  kev_pargenlexer_init(&lex, file);

  do {
    kev_pargenlexer_next(&lex);
    printf("%s\n", kev_pargenlexer_info(lex.currtoken.kind));
  } while (lex.currtoken.kind != KEV_PTK_END);

  kev_pargenlexer_destroy(&lex);
  fclose(file);
  return 0;
}
