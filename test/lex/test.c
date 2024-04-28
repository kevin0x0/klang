#include "klang/include/parse/kllex.h"

void print_token(KlLex* lex);



int main(void) {
  const char* filename = "test.txt";
  KlStrTab* strtab = klstrtab_create();
  Ki* input = kifile_create(filename);
  Ko* err = kofile_attach(stderr);
  KlLex* lex = kllex_create(input, err, filename, strtab);

  kllex_next(lex);
  while (lex->tok.kind != KLTK_END) {
    print_token(lex);
    kllex_next(lex);
  }

  kllex_delete(lex);
  klstrtab_delete(strtab);
  ki_delete(input);
  ko_delete(err);
  return 0;
}

void print_token(KlLex* lex) {
  switch (lex->tok.kind) {
    case KLTK_ID: {
      printf("identifier: %.*s\n", (int)(lex->tok.string.length), klstrtab_getstring(lex->strtab, lex->tok.string.id));
      break;
    }
    case KLTK_STRING: {
      printf("string: %.*s\n", (int)(lex->tok.string.length), klstrtab_getstring(lex->strtab, lex->tok.string.id));
      break;
    }
    case KLTK_INT: {
      printf("integer: %zd\n", lex->tok.intval);
      break;
    }
    case KLTK_BOOLVAL: {
      printf("boolean: %s\n", lex->tok.boolval == KL_TRUE ? "true" : "false");
      break;
    }
    default: {
      printf("%s\n", kltoken_desc(lex->tok.kind));
      break;
    }
  }
}
