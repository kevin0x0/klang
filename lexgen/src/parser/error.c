#include "lexgen/include/parser/error.h"
#include "lexgen/include/parser/lexer.h"
#include <stdlib.h>

void kev_parser_error_report(FILE* err_stream, FILE* infile, char* info, size_t position) {
  size_t count = 0;
  size_t line_begin = 0;
  size_t line_no = 1;
  while (count++ < position) {
    int ch = 0;
    if ((ch = getc(infile)) == EOF) {
      break;
    }
    else if (ch == '\n') {
      line_begin = ftell(infile);
      line_no++;
    }
  }
  fprintf(err_stream, "error in line %u: %s\n", (unsigned int)line_no, info);
  fseek(infile, line_begin, SEEK_SET);
  int ch = 0;
  while ((ch = fgetc(infile)) != '\n' && ch != EOF) {
    fputc(ch, err_stream);
  }
  fputc('\n', err_stream);
  fseek(infile, line_begin, SEEK_SET);
  count = line_begin;
  while (count++ < position) {
    ch = fgetc(infile);
    if (ch == '\t') fputc(ch, err_stream);
  }
  fputc('^', err_stream);
  fputc('\n', err_stream);
}

void kev_parser_error_handling(FILE* err_stream, KevLexGenLexer* lex, int kind) {
  KevLexGenToken token;
  do {
    kev_lexgenlexer_next(lex, &token);
  } while (token.kind != kind && token.kind != KEV_LEXGEN_TOKEN_END);
  if (token.kind != kind) {
    fputs("fatal: can not recovery from last error, terminated\n", err_stream);
    exit(EXIT_FAILURE);
  }
}


