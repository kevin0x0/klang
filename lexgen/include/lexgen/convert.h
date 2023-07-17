#ifndef KEVCC_LEXGEN_INCLUDE_LEXGEN_CONVERT_H
#define KEVCC_LEXGEN_INCLUDE_LEXGEN_CONVERT_H

#include "lexgen/include/parser/parser.h"

#include <stdio.h>

typedef struct tagPatternBinary {
  size_t pattern_no;
  size_t nfa_no;
  size_t dfa_non_acc_no;
  size_t dfa_state_no;
  size_t dfa_start;
  char** callbacks;
  char** macros;
  char** infos;
  int* pattern_mapping;
  void* table;
  size_t charset_size;
  size_t state_length;
} KevPatternBinary;

void kev_lexgen_convert(KevPatternBinary* binary_info, KevParserState* parser_state);
void kev_lexgen_convert_destroy(KevPatternBinary* binary_info);

#endif