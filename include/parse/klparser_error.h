#ifndef _KLANG_INCLUDE_PARSE_KLPARSER_ERROR_H_
#define _KLANG_INCLUDE_PARSE_KLPARSER_ERROR_H_

#include "include/parse/kllex.h"
#include "include/parse/klparser.h"
#include "include/parse/kltokens.h"
#include <stdbool.h>

void klparser_error(KlParser* parser, Ki* input, KlFileOffset begin, KlFileOffset end, const char* format, ...);
void* klparser_error_oom(KlParser* parser, KlLex* lex);

#endif


