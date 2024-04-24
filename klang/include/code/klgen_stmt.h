#ifndef KEVCC_KLANG_INCLUDE_CODE_KLGEN_STMT_H
#define KEVCC_KLANG_INCLUDE_CODE_KLGEN_STMT_H

#include "klang/include/code/klgen.h"
#include "klang/include/code/klgen_expr.h"
#include "klang/include/cst/klcst_stmt.h"



void klgen_stmtlist(KlGenUnit* gen, KlCstStmtList* cst);
bool klgen_stmtblock(KlGenUnit* gen, KlCstStmtList* stmtlist);
/* do not allow continue or break out of this scope */
bool klgen_stmtblockpure(KlGenUnit* gen, KlCstStmtList* stmtlist);
/* do not allow continue or break out of this scope */
void klgen_stmtlistpure(KlGenUnit* gen, KlCstStmtList* stmtlist);
void klgen_assignfrom(KlGenUnit* gen, KlCst* lval, size_t stkid);

#endif