#ifndef _KLANG_INCLUDE_PARSE_KLLEX_H_
#define _KLANG_INCLUDE_PARSE_KLLEX_H_

#include "include/ast/klast.h"
#include "include/ast/klstrtbl.h"
#include "include/parse/kltokens.h"
#include "include/error/klerror.h"
#include "deps/k/include/kio/ki.h"


#define KLLEX_STRLIMIT        (KLSTRTAB_EXTRA)

#define kllex_tokbegin(lex)   ((lex)->tok.begin)
#define kllex_tokend(lex)     ((lex)->tok.end)

typedef struct tagKlLex {
  KlStrTbl* strtbl;         /* string table */
  Ki* input;                /* input stream */
  char* inputname;          /* name of input stream */
  size_t nerror;            /* number of lexical error */
  size_t currline;          /* current line number */
  KlError* klerror;         /* error reporter */
  struct {
    KlFileOffset begin;     /* begin position of this token */
    KlFileOffset end;       /* end position of this token */
    KlTokenKind kind;
    union {
      KlCInt intval;
      KlCFloat floatval;
      KlCBool boolval;
      KlStrDesc string;
    };
    bool hasleadingblank;   /* whether there is blank before this token */
  } tok;                    /* token information */
} KlLex;

bool kllex_init(KlLex* lex, Ki* ki, KlError* klerr, const char* inputname, KlStrTbl* strtbl);
void kllex_destroy(KlLex* lex);
KlLex* kllex_create(Ki* ki, KlError* klerr, const char* inputname, KlStrTbl* strtbl);
void kllex_delete(KlLex* lex);

void kllex_next(KlLex* lex);
static inline void kllex_setcurrtok(KlLex* lex, KlTokenKind kind, KlFileOffset begin, KlFileOffset end);
static inline bool kllex_hasleadingblank(const KlLex* lex);
static inline bool kllex_trymatch(KlLex* lex, KlTokenKind kind);
static inline bool kllex_check(const KlLex* lex, KlTokenKind kind);
static inline KlTokenKind kllex_tokkind(const KlLex* lex);
static inline Ki* kllex_inputstream(const KlLex* lex);



static inline void kllex_setcurrtok(KlLex* lex, KlTokenKind kind, KlFileOffset begin, KlFileOffset end) {
  lex->tok.kind = kind;
  lex->tok.begin = begin;
  lex->tok.end = end;
}

static inline bool kllex_hasleadingblank(const KlLex* lex) {
  return lex->tok.hasleadingblank;
}

static inline bool kllex_trymatch(KlLex* lex, KlTokenKind kind) {
  if (lex->tok.kind == kind) {
    kllex_next(lex);
    return true;
  }
  return false;
}

static inline bool kllex_check(const KlLex* lex, KlTokenKind kind) {
  return kllex_tokkind(lex) == kind;
}

static inline KlTokenKind kllex_tokkind(const KlLex* lex) {
  return lex->tok.kind;
}

static inline Ki* kllex_inputstream(const KlLex* lex) {
  return lex->input;
}

#endif
