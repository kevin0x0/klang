#include "lex1.h"
#include <iostream>

int main(int argc, char** argv) {
  tokenizer tkzer("test.txt");
  tokenizer::Token token;
  while (true) {
    tkzer.next(token);
    std::cout << tkzer.get_info(token.kind) << std::endl;
    if (token.kind == 3) break;
  }
  return 0;
}
