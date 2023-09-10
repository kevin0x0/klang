#include "pargen/include/parser/error.h"

void kev_parser_throw_error(FILE* out, FILE* infile, size_t begin, const char* info1, const char* info2) {
  if (!infile) {
    fprintf(out, "%s", info1);
    if (info2)
      fprintf(out, "%s", info2);
    return;
  }

  size_t original_position = ftell(infile);
  fseek(infile, 0, SEEK_SET);
  size_t count = 0;
  size_t line_begin = 0;
  size_t line_begin_count = 0;
  size_t line_no = 1;
  while (count++ < begin) {
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
  fprintf(out, "error in line %u: %s", (unsigned int)line_no, info1);
  if (info2)
    fprintf(out, "%s", info2);
  fputc('\n', out);
  fseek(infile, line_begin, SEEK_SET);
  int ch = 0;
  while ((ch = fgetc(infile)) != '\n' && ch != EOF) {
    fputc(ch, out);
  }
  fputc('\n', out);
  fseek(infile, line_begin, SEEK_SET);
  count = line_begin;
  count = line_begin_count;
  while (count++ < begin) {
    ch = fgetc(infile);
    if (ch == '\t')
      fputc('\t', out);
    else
      fputc(' ', out);

  }
  fputc('^', out);
  fputc('\n', out);
  fseek(infile, original_position, SEEK_SET);
}
