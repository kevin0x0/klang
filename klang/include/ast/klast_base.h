#ifndef KEVCC_KLANG_INCLUDE_AST_KLAST_BASE_H
#define KEVCC_KLANG_INCLUDE_AST_KLAST_BASE_H


typedef enum tagKlAstNodeType {
  KLAST_EXPR_BIN,     /* binary operator */
  KLAST_EXPR_POST,    /* postfix operator */
  KLAST_EXPR_PRE,     /* prefix operator */
  KLAST_EXPR_UNIT,
  KLAST_STMT_AS,
  KLAST_STMT_EXPR,
  KLAST_STMT_LET,
} KlAstNodeType;

/* this serves as the base class of abstract syntax tree node,
 * and will be contained to the header of any other node.
 */

typedef struct tagKlAstBase {
  KlAstNodeType node_type;
} KlAstBase;


#endif
