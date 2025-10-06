#ifndef _KLANG_INCLUDE_CODE_KLCODE_H_
#define _KLANG_INCLUDE_CODE_KLCODE_H_

#include "include/ast/klast.h"
#include "include/ast/klstrtbl.h"
#include "include/error/klerror.h"
#include "include/common/klinst.h"


typedef struct tagKlCode KlCode;
typedef struct tagKlFilePosition {
  unsigned begin;
  unsigned end;
} KlFilePosition;

typedef struct tagKlCodeGenConfig {
  KlStrDesc srcfile;
  const char* inputname;
  KlError* klerr;
  Ki* input;
  bool debug;
  bool posinfo;
} KlCodeGenConfig;

/* this is the same structure as KlRefInfo used in compiler */
typedef struct tagKlCRefInfo {
  unsigned index;
  bool on_stack;
} KlCRefInfo;

struct tagKlCode {
  KlCRefInfo* refinfo;
  KlConstant* constants;
  KlInstruction* code;
  KlFilePosition* posinfo;
  KlStrDesc srcfile;
  KlCode** nestedfunc;        /* functions created inside this function */
  KlStrTbl* strtbl;
  unsigned nref;
  unsigned nconst;
  unsigned codelen;
  unsigned nnested;
  unsigned nparam;            /* number of parameters */
  unsigned framesize;         /* stack frame size of this klang function */
};

KlCode* klcode_create(KlCRefInfo* refinfo, unsigned nref, KlConstant* constants, unsigned nconst,
                      KlInstruction* code, KlFilePosition* posinfo, unsigned codelen,
                      KlCode** nestedfunc, unsigned nnested, KlStrTbl* strtbl, unsigned nparam,
                      unsigned framesize, KlStrDesc srcfile);
void klcode_delete(KlCode* code);

KlCode* klcode_create_fromast(KlAstStmtList* ast, KlStrTbl* strtbl, KlCodeGenConfig* config);


typedef enum tagKlUnDumpError {
  KLUNDUMP_SUCCESS = 0,
  KLUNDUMP_ERROR_MAGIC,
  KLUNDUMP_ERROR_ENDIAN,
  KLUNDUMP_ERROR_VERSION,
  KLUNDUMP_ERROR_SIZE,
  KLUNDUMP_ERROR_BAD,
  KLUNDUMP_ERROR_OOM,
} KlUnDumpError;

bool klcode_dump(const KlCode* code, Ko* file);
KlCode* klcode_undump(Ki* file, KlStrTbl* strtbl, KlUnDumpError* error);
void klcode_print(const KlCode* code, Ko* out);

#endif
