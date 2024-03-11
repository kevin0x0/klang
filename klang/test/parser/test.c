#include "klang/include/parse/klparser.h"
#include "klang/include/parse/kllex.h"

void loop(void);


int main(void) {
  loop();
  return 0;
}

void loop(void) {
  const char* filename = "test.kl";
  KlStrTab* strtab = klstrtab_create();
  Ki* input = kifile_create(filename);
  Ko* err = kofile_attach(stderr);
  KlLex* lex = kllex_create(input, err, filename, strtab);
  KlParser parser;
  parser.err = err;
  parser.errcount = 0;
  parser.inputname = (char*)filename;
  parser.config.curl = '~';
  parser.config.tabstop = 8;

  kllex_next(lex);
  KlCst* expr = klparser_stmtlist(&parser, lex);

  if (expr) {
    printf("maybe success... root kind : %d\n", klcst_kind(expr));
  }

  if (expr) klcst_delete(expr);
  kllex_delete(lex);
  klstrtab_delete(strtab);
  ki_delete(input);
  ko_delete(err);
}
