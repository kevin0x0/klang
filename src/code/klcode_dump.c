#include "include/code/klcode.h"

struct a {
  KlCRefInfo* refinfo;
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

#define KLCODEDUMP_MAGIC_SZ         (4)
#define KLCODEDUMP_BIG_ENDIAN       (0)
#define KLCODEDUMP_SMALL_ENDIAN     (1)
#define KLCODEDUMP_VERSION          (0)

typedef unsigned char uchar;

typedef struct tafKlCodeDumpHeader {
  char magic[KLCODEDUMP_MAGIC_SZ];
  uchar endian;
  uchar version;

} KlCodeDumpHeader;

void klcode_dump(KlCode* code, Ko* file) {
}

KlCode* klcode_create_fromfile(Ki* file);
