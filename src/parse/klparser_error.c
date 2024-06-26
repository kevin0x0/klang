#include "include/parse/klparser_error.h"
#include "include/error/klerror.h"

void* klparser_error_oom(KlParser* parser, KlLex* lex) {
  klparser_error(parser, kllex_inputstream(lex), lex->tok.begin, lex->tok.end, "out of memory");
  return NULL;
}

void klparser_error(KlParser* parser, Ki* input, KlFileOffset begin, KlFileOffset end, const char* format, ...) {
  kl_assert(begin != KLPARSER_ERROR_PH_FILEPOS && end != KLPARSER_ERROR_PH_FILEPOS, "position of a syntax tree not set!");
  va_list args;
  va_start(args, format);
  klerror_errorv(parser->klerror, input, parser->inputname, begin, end, format, args);
  va_end(args);
}
