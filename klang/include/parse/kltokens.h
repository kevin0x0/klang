#ifndef KEVCC_KLANG_INCLUDE_PARSE_KLTOKENS_H
#define KEVCC_KLANG_INCLUDE_PARSE_KLTOKENS_H

typedef enum tagKlToken {
  KLTK_ERR = -1,
  KLTK_ID = 0,
  KLTK_INT,
  KLTK_STRING,
  KLTK_BOOLVAL,
  KLTK_NIL,
  KLTK_VARARGS,
  /* operators */
  KLTK_ADD,
  KLTK_MINUS,
  KLTK_MUL,
  KLTK_DIV,
  KLTK_MOD,
  KLTK_CONCAT,
  KLTK_DOT,
  /* compare */
  KLTK_LT,
  KLTK_LE,
  KLTK_GT,
  KLTK_GE,
  KLTK_EQ,
  KLTK_NE,

  KLTK_NOT,
  KLTK_AND,
  KLTK_OR,

  KLTK_LPAREN,
  KLTK_RPAREN,
  KLTK_LBRACKET,
  KLTK_RBRACKET,
  KLTK_LBRACE,
  KLTK_RBRACE,

  KLTK_COMMA,
  KLTK_SEMI,
  KLTK_COLON,
  KLTK_QUESTION,
  KLTK_ARRAW,

  KLTK_ASSIGN,
  KLTK_BAR,

  /* keywords */
  KLTK_IF,
  KLTK_ELSE,
  KLTK_WHILE,
  KLTK_REPEAT,
  KLTK_UNTIL,
  KLTK_FOR,
  KLTK_IN,
  KLTK_LET,
  KLTK_LOCAL,
  KLTK_SHARED,
  KLTK_CLASS,
  KLTK_RETURN,
  KLTK_BREAK,
  KLTK_CONTINUE,

  KLTK_END,
  KLTK_NTOKEN,
} KlToken;

const char* kltoken_desc(KlToken kind);


#endif
