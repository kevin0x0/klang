#ifndef KEVCC_PARGEN_INCLUDE_PARSER_ERROR_H
#define KEVCC_PARGEN_INCLUDE_PARSER_ERROR_H

#include "utils/include/general/global_def.h"

#include <stdio.h>

void kev_parser_throw_error(FILE* out, FILE* infile, const char* filename, size_t begin, const char* info1, const char* info2);

#endif
