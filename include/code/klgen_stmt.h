#ifndef KEVCC_KLANG_INCLUDE_CODE_KLGEN_STMT_H
#define KEVCC_KLANG_INCLUDE_CODE_KLGEN_STMT_H

#include "include/code/klgen.h"
#include "include/code/klgen_expr.h"
#include "include/ast/klast.h"



void klgen_stmtlist(KlGenUnit* gen, KlAstStmtList* ast);
bool klgen_stmtblock(KlGenUnit* gen, KlAstStmtList* stmtlist);
/* do not allow continue or break out of this scope */
bool klgen_stmtblockpure(KlGenUnit* gen, KlAstStmtList* stmtlist);
/* do not allow continue or break out of this scope */
void klgen_stmtlistpure(KlGenUnit* gen, KlAstStmtList* stmtlist);
void klgen_assignfrom(KlGenUnit* gen, KlAst* lval, size_t stkid);

#endif
