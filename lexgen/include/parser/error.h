#ifndef KEVCC_LEXGEN_INCLUDE_PARSER_ERROR_H
#define KEVCC_LEXGEN_INCLUDE_PARSER_ERROR_H

#include "lexgen/include/parser/lexer.h"
#include <stdio.h>
void kev_parser_error_report(FILE* err, FILE* infile, char* info, size_t begin);
void kev_parser_error_handling(FILE* err, KevLexGenLexer* lex, int kind);


#endif
