#ifndef KEVCC_LEXGEN_INCLUDE_LEXGEN_CONVERT_H
#define KEVCC_LEXGEN_INCLUDE_LEXGEN_CONVERT_H

#include "lexgen/include/parser/parser.h"

#include <stdio.h>

/* this structure store the meory representation of transition table and
 * other infomation. */
typedef struct tagLTableInfos {
  size_t pattern_no;
  size_t nfa_no;
  size_t dfa_non_acc_no;
  size_t dfa_state_no;
  size_t dfa_start;
  char** callbacks;
  KArray** macros;  /* store macro in order of patternlist */
  int* macro_ids;
  char** infos;
  int* pattern_mapping;   /* DFA state to pattern_id */
  void* table;
  size_t charset_size;
  size_t state_length;
} KevLTableInfos;

/* fill structure 'binary_info' according to infomation in 'parser_state' */
void kev_lexgen_convert(KevLTableInfos* table_info, KevLParserState* parser_state);
/* free resources */
void kev_lexgen_convert_destroy(KevLTableInfos* table_info);

#endif
