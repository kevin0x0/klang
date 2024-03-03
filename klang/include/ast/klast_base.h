#ifndef KEVCC_KLANG_INCLUDE_AST_KLAST_BASE_H
#define KEVCC_KLANG_INCLUDE_AST_KLAST_BASE_H

#include "klang/include/ast/klast.h"

/* this serves as the base class of abstract syntax tree node,
 * and will be contained to the header of any other node.
 */

typedef struct tagKlAstBase {
  KlAstNodeType type;
} KlAstBase;


#endif
