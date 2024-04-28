#include "klang/include/code/klcode.h"
#include "klang/include/error/klerror.h"
#include "klang/include/parse/klparser.h"
#include <time.h>

void codegen_test(KlStrTbl* strtbl, Ki* input, const char* inputname, KlError* klerr, KlCst* cst);

int main(void) {
  const char* filename = "test.kl";
  KlStrTbl* strtbl = klstrtbl_create();
  Ki* input = kifile_create(filename);
  Ko* err = kofile_attach(stderr);
  KlParser parser;
  KlError klerr = { .err = err, .errcount = 0, .config = {
    .curl = '~',
    .tabstop = 8,
    .zerocurl = '^',
    .maxtextline = 3,
    .promptnorm = "|| ",
    .prompttext = "||== ",
    .promptmsg = "|| "
  }};
  KlLex* lex = kllex_create(input, &klerr, filename, strtbl);
  klparser_init(&parser, lex->strtbl, (char*)filename, &klerr);
  kllex_next(lex);
  KlCst* expr = klparser_stmtlist(&parser, lex);
  if (expr) klcst_setposition(expr, 0, lex->tok.begin);

  if (klerr.errcount == 0) {
    codegen_test(strtbl, input, filename, &klerr, expr);
  }

  if (expr) klcst_delete(expr);
  kllex_delete(lex);
  klstrtbl_delete(strtbl);
  ki_delete(input);
  ko_delete(err);
  return 0;
}

void codegen_test(KlStrTbl* strtbl, Ki* input, const char* inputname, KlError* klerr, KlCst* cst) {
  KlCode* code = klcode_create_fromcst(cst, strtbl, input, inputname, klerr, true);
  if (kl_unlikely(!code)) return;
  Ko* ko = kofile_attach(stdout);
  klcode_print(code, ko);
  ko_delete(ko);
  klcode_delete(code);
}
