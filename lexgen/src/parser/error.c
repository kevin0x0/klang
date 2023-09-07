#include "lexgen/include/parser/error.h"

#include <stdio.h>
#include <stdlib.h>

void kev_parser_error_report(FILE* err_stream, FILE* infile, const char* info, size_t position) {
  if (!infile) {
    fprintf(err_stream, "%s", info);
    return;
  }

  size_t original_position = ftell(infile);
  fseek(infile, 0, SEEK_SET);
  size_t count = 0;
  size_t line_begin = 0;
  size_t line_begin_count = 0;
  size_t line_no = 1;
  while (count++ < position) {
    int ch = 0;
    if ((ch = getc(infile)) == EOF) {
      break;
    }
    else if (ch == '\n') {
      line_begin = ftell(infile);
      line_begin_count = count;
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
  count = line_begin_count;
  while (count++ < position) {
    ch = fgetc(infile);
    if (ch == '\t')
      fputc('\t', err_stream);
    else
      fputc(' ', err_stream);

  }
  fputc('^', err_stream);
  fputc('\n', err_stream);
  fseek(infile, original_position, SEEK_SET);
}

void kev_parser_error_handling(FILE* err_stream, KevLLexer* lex, int kind, bool complement) {
  KevLToken token;
  do {
    kev_lexgenlexer_next(lex, &token);
  } while ((token.kind == kind) == complement && token.kind != KEV_LTK_END);
  if ((token.kind == kind) == complement) {
    fputs("fatal: can not recovery from last error, terminated\n", err_stream);
    exit(EXIT_FAILURE);
  }
}


