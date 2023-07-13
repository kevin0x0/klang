#include "lex.h"
#include <stdio.h>

int main(int argc, char** argv) {
  Lex lex;
  lex_init(&lex, "test.txt");
  Token token;
  while (true) {
    lex_next(&lex, &token);
    char tmp = lex.buffer[token.end];
    lex.buffer[token.end] = '\0';
    printf("%s %s\n", lex.buffer + token.begin, lex_get_info(&lex, token.kind));
    lex.buffer[token.end] = tmp;
    if (token.kind == 3) break;
  }
  lex_destroy(&lex);
  return 0;
}
