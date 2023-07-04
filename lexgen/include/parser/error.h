#ifndef KEVCC_LEXGEN_INCLUDE_PARSER_ERROR_H
#define KEVCC_LEXGEN_INCLUDE_PARSER_ERROR_H

#include "lexgen/include/parser/lexer.h"
#include "lexgen/include/general/global_def.h"
#include <stdio.h>

void kev_parser_error_report(FILE* err, FILE* infile, char* info, size_t position);
void kev_parser_error_handling(FILE* err, KevLexGenLexer* lex, int kind, bool complement);


#endif
