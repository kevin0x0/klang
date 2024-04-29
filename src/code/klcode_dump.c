#include "include/code/klcode.h"

struct a {
  KlRefInfo* refinfo;
  size_t nref;
  KlConstant* constants;
  size_t nconst;
  KlInstruction* code;
  size_t codelen;
  KlFilePosition* posinfo;
  KlCode** nestedfunc;        /* functions created inside this function */
  size_t nnested;
  KlStrTbl* strtbl;
  size_t nparam;              /* number of parameters */
  size_t framesize;           /* stack frame size of this klang function */
};

typedef struct tafKlCodeDumpHeader {
  char magic[4];
} KlCodeDumpHeader;

void klcode_dump(KlCode* code, Ko* file) {
}

KlCode* klcode_create_fromfile(Ki* file);
