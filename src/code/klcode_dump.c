#include "include/code/klcode.h"
#include <setjmp.h>

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

#define KLCODEDUMP_MAGIC_SZ         (8)
#define KLCODEDUMP_MAGIC            ("\aklang")
#define KLCODEDUMP_BIG_ENDIAN       (0)
#define KLCODEDUMP_SMALL_ENDIAN     (1)
#define KLCODEDUMP_VERSION          (0)

typedef unsigned char uchar;

typedef struct tagKlCodeDumpHeader {
  char magic[KLCODEDUMP_MAGIC_SZ];
  uchar endian;
  uchar version;
  uchar unsignedsize;           /* sizeof (unsigned) */
  uchar intsize;                /* sizeof (KlCInt) */
  uchar floatsize;              /* sizeof (KlCFloat) */
  uchar reserved[32 - (KLCODEDUMP_MAGIC_SZ + 5)];
} KlCodeDumpHeader;

typedef struct tagKlCodeHeader {
  unsigned nref;
  unsigned nconst;
  unsigned codelen;
  unsigned nnested;
  unsigned nparam;
  unsigned framesize;
  uchar hasposinfo;
  uchar reserved[4];
} KlCodeHeader;

typedef struct tagKlCodeDumpState {
  Ko* file;
  jmp_buf jmppos;
} KlCodeDumpState;

kl_noreturn static void klcode_dump_failure(KlCodeDumpState* state);
kl_noreturn static void klcode_dump_code(KlCodeDumpState* state, KlCode* code);
static void klcode_dump_codeheader(KlCodeDumpState* state, KlCodeHeader* header);


bool klcode_dump(KlCode* code, Ko* file) {
}

KlCode* klcode_create_fromfile(Ki* file);


static void klcode_dump_codeheader(KlCodeDumpState* state, KlCodeHeader* header) {
  if (kl_unlikely(sizeof (KlCodeHeader) != ko_write(state->file, header, sizeof (KlCodeHeader))))
    klcode_dump_failure(state);
}

kl_noreturn static void klcode_dump_code(KlCodeDumpState* state, KlCode* code) {
  KlCodeHeader header = {
    .nref = code->nref,
    .nconst = code->nconst,
    .nparam = code->nparam,
    .nnested = code->nnested,
    .codelen = code->codelen,
    .framesize = code->framesize,
    .hasposinfo = code->posinfo != NULL,
  };
  memset(header.reserved, 0, sizeof (header.reserved));
  klcode_dump_codeheader(state, &header);
}

kl_noreturn static void klcode_dump_failure(KlCodeDumpState* state) {
  longjmp(state->jmppos, 1);
}
