#include "include/code/klcode.h"
#include "include/error/klerror.h"
#include "include/parse/klparser.h"
#include "deps/k/include/kio/kifile.h"
#include "deps/k/include/kio/kofile.h"
#include <time.h>

void codegen_test(KlStrTbl* strtbl, Ki* input, const char* inputname, KlError* klerr, KlAstStmtList* ast);

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
  KlAstStmtList* file = klparser_stmtlist(&parser, lex);
  if (file) klast_setposition(file, 0, lex->tok.begin);

  if (klerr.errcount == 0) {
    codegen_test(strtbl, input, filename, &klerr, file);
  }

  if (file) klast_delete(file);
  kllex_delete(lex);
  klstrtbl_delete(strtbl);
  ki_delete(input);
  ko_delete(err);
  return 0;
}

void codegen_test(KlStrTbl* strtbl, Ki* input, const char* inputname, KlError* klerr, KlAstStmtList* ast) {
  KlCodeGenConfig config = { .posinfo = true, .klerr = klerr, .inputname = inputname, .input = input, .debug = false };
  KlCode* code = klcode_create_fromast(ast, strtbl, &config);
  if (kl_unlikely(!code)) return;
  Ko* ko = kofile_attach(stdout);
  klcode_print(code, ko);
  ko_delete(ko);
  klcode_delete(code);
}
