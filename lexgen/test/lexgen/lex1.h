#ifndef __KEVCC_LEXGEN_LEXER_H
#define __KEVCC_LEXGEN_LEXER_H

#include <stdint.h>
#include <stddef.h>
#include <string>


class tokenizer {
public:
  
  struct TokenAttr {
    int64_t ival;
    double fval;
    char* sval;
  };
  
  
  struct Token {
    size_t begin;
    size_t end;
    int kind;
    TokenAttr attr;
  };
  
  using TransTab = uint8_t (*)[256];
  using Callback = void(Token&, uint8_t*);

private:
  constexpr static int dead_state = 255;

  uint8_t* buffer;
  TransTab table;
  int* patterns;
  size_t start_state;
  Callback** callbacks;
public:
  tokenizer(const std::string& filepath);
  ~tokenizer();
  void next(Token& token);
  static std::string get_info(int kind);
};



#endif
