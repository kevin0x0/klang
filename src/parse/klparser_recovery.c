#include "include/parse/klparser_recovery.h"

/* recovery */
bool klparser_discarduntil(KlLex* lex, KlTokenKind kind) {
  while (lex->tok.kind != kind) {
    if (lex->tok.kind == KLTK_END) return false;
    kllex_next(lex);
  }
  return true;
}

bool klparser_discarduntil2(KlLex* lex, KlTokenKind kind1, KlTokenKind kind2) {
  while (lex->tok.kind != kind1 && lex->tok.kind != kind2) {
    if (lex->tok.kind == KLTK_END) return false;
    kllex_next(lex);
  }
  return true;
}

bool klparser_discardto(KlLex* lex, KlTokenKind kind) {
  bool success = klparser_discarduntil(lex, kind);
  if (success) kllex_next(lex);
  return success;
}

bool klparser_discardto2(KlLex* lex, KlTokenKind kind1, KlTokenKind kind2) {
  bool success = klparser_discarduntil2(lex, kind1, kind2);
  if (success) kllex_next(lex);
  return success;
}

