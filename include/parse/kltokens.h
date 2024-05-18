#ifndef _KLANG_INCLUDE_PARSE_KLTOKENS_H_
#define _KLANG_INCLUDE_PARSE_KLTOKENS_H_

typedef enum tagKlTokenKind {
  KLTK_ERR = -1,
  KLTK_ID = 0,
  KLTK_INT,
  KLTK_INTDOT,
  KLTK_FLOAT,
  KLTK_STRING,
  KLTK_BOOLVAL,
  KLTK_NIL,
  KLTK_VARARG,
  /* operators */
  KLTK_CONCAT,
  KLTK_ADD,
  KLTK_MINUS,
  KLTK_MUL,
  KLTK_DIV,
  KLTK_MOD,
  KLTK_IDIV,
  KLTK_APPEND,
  /* compare */
  KLTK_LT,
  KLTK_LE,
  KLTK_GT,
  KLTK_GE,
  KLTK_EQ,
  KLTK_NE,
  KLTK_IS,
  KLTK_ISNOT,
  /* boolean */
  KLTK_AND,
  KLTK_OR,
  KLTK_NOT,

  KLTK_LEN,

  KLTK_LPAREN,
  KLTK_RPAREN,
  KLTK_LBRACKET, KLTK_INDEX = KLTK_LBRACKET,
  KLTK_RBRACKET,
  KLTK_LBRACE, KLTK_CALL = KLTK_LBRACE,
  KLTK_RBRACE,

  KLTK_DOT,
  KLTK_COMMA,
  KLTK_SEMI,
  KLTK_COLON,
  KLTK_QUESTION,
  KLTK_DARROW,
  KLTK_ARROW,

  KLTK_ASSIGN,
  KLTK_BAR,

  /* keywords */
  KLTK_CASE,
  KLTK_OF,
  KLTK_WHERE,
  KLTK_IF,
  KLTK_ELSE,
  KLTK_WHILE,
  KLTK_REPEAT,
  KLTK_UNTIL,
  KLTK_FOR,
  KLTK_IN,
  KLTK_LET,
  KLTK_RETURN,
  KLTK_BREAK,
  KLTK_CONTINUE,
  KLTK_LOCAL,
  KLTK_SHARED,
  KLTK_NEW,
  KLTK_INHERIT,
  KLTK_METHOD,
  KLTK_ASYNC,
  KLTK_YIELD,

  KLTK_END,
  KLTK_NTOKEN,
} KlTokenKind;

#define kltoken_isbinop(kind)     ((kind) >= KLTK_CONCAT && (kind) <= KLTK_OR)
#define kltoken_isarith(kind)     ((kind) >= KLTK_ADD && (kind) <= KLTK_IDIV)
#define kltoken_isrelation(kind)  ((kind) >= KLTK_LT && (kind) <= KLTK_ISNOT)

const char* kltoken_desc(KlTokenKind kind);

#endif
