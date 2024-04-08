#include "klang/include/parse/klparser.h"
#include "klang/include/parse/kllex.h"

void loop(void);


int main(void) {
  //while (true) {
    loop();
  //}
  return 0;
}

void loop(void) {
  const char* filename = "test.kl";
  KlStrTab* strtab = klstrtab_create();
  Ki* input = kifile_create(filename);
  Ko* err = kofile_attach(stderr);
  KlParser parser;
  KlError klerr = { .err = err, .errcount = 0, .config = { .curl = '~', .tabstop = 8, .zerocurl = '^' } };
  KlLex* lex = kllex_create(input, &klerr, filename, strtab);
  klparser_init(&parser, lex->strtab, (char*)filename, &klerr);
  kllex_next(lex);
  KlCst* expr = klparser_stmtlist(&parser, lex);

  if (klerr.errcount == 0) {
    printf("root kind : %d\n", klcst_kind(expr));
  }

  if (expr) klcst_delete(expr);
  kllex_delete(lex);
  klstrtab_delete(strtab);
  ki_delete(input);
  ko_delete(err);
}
