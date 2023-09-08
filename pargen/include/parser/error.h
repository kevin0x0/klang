#ifndef KEVCC_PARGEN_INCLUDE_PARSER_ERROR_H
#define KEVCC_PARGEN_INCLUDE_PARSER_ERROR_H

#include "utils/include/general/global_def.h"

#include <stdio.h>

void kev_parser_error_report(FILE* out, FILE* infile, size_t begin, const char* info);

#endif
