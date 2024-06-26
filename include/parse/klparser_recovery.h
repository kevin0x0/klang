#ifndef _KLANG_INCLUDE_PARSE_KLPARSER_RECOVERY_H_
#define _KLANG_INCLUDE_PARSE_KLPARSER_RECOVERY_H_

#include "include/parse/kllex.h"
#include "include/parse/kltokens.h"
#include <stdbool.h>

bool klparser_discarduntil(KlLex* lex, KlTokenKind kind);
bool klparser_discarduntil2(KlLex* lex, KlTokenKind kind1, KlTokenKind kind2);
bool klparser_discardto(KlLex* lex, KlTokenKind kind);
bool klparser_discardto2(KlLex* lex, KlTokenKind kind1, KlTokenKind kind2);

#endif

