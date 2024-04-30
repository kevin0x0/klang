#include "include/code/klcode.h"
#include "include/cst/klcst.h"
#include "include/cst/klstrtbl.h"
#include "include/lang/klinst.h"
#include "include/misc/klutils.h"
#include <limits.h>
#include <setjmp.h>

#define KLCODEDUMP_MAGIC_SZ         (8)
#define KLCODEDUMP_MAGIC            ("\aklang")
#define KLCODEDUMP_BIG_ENDIAN       (0)
#define KLCODEDUMP_SMALL_ENDIAN     (1)
#define KLCODEDUMP_VERSION          (0)

typedef unsigned char uchar;

typedef struct tagKlCodeGlobalHeader {
  char magic[KLCODEDUMP_MAGIC_SZ];
  uchar endian;
  uchar version;
  uchar unsignedsize;           /* sizeof (unsigned) */
  uchar intsize;                /* sizeof (KlCInt) */
  uchar floatsize;              /* sizeof (KlCFloat) */
  uchar reserved[32 - (KLCODEDUMP_MAGIC_SZ + 5)];
} KlCodeGlobalHeader;

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



/* dump */
typedef struct tagKlCodeDumpState KlCodeDumpState;
struct tagKlCodeDumpState {
  Ko* file;
  jmp_buf jmppos;
};

kl_noreturn static void klcode_dump_failure(KlCodeDumpState* state);
static void klcode_dump_code(KlCodeDumpState* state, KlCode* code);
static void klcode_dumpobj(KlCodeDumpState* state, void* obj, size_t size);
static void klcode_dump_globalheader(KlCodeDumpState* state, KlCode* code);
static void klcode_dump_codeheader(KlCodeDumpState* state, KlCodeHeader* header);
static void klcode_dump_refinfo(KlCodeDumpState* state, KlCRefInfo* refinfo, size_t nref);
static void klcode_dump_constants(KlCodeDumpState* state, KlStrTbl* strtbl, KlConstant* constants, size_t nconst);
static void klcode_dump_instructions(KlCodeDumpState* state, KlInstruction* inst, size_t codelen);
static void klcode_dump_posinfo(KlCodeDumpState* state, KlFilePosition* posinfo, size_t npos);


bool klcode_dump(KlCode* code, Ko* file) {
  KlCodeDumpState dumpstate = {
    .file = file,
  };
  if (setjmp(dumpstate.jmppos) == 0) {
    klcode_dump_globalheader(&dumpstate, code);
    klcode_dump_code(&dumpstate, code);
    return true;
  } else {
    return false;
  }
}

static void klcode_dumpobj(KlCodeDumpState* state, void* obj, size_t size) {
  if (kl_unlikely(ko_write(state->file, obj, size) != size))
    klcode_dump_failure(state);
}

static bool isbigendian(void) {
  kl_static_assert(sizeof (unsigned) > 1, "that is impossible");
  unsigned char a[sizeof (unsigned)];
  unsigned b = 1;
  memcpy(a, &b, sizeof (unsigned));
  return a[0] == 0;
}

static void klcode_dump_globalheader(KlCodeDumpState* state, KlCode* code) {
  KlCodeGlobalHeader header = {
    .magic = KLCODEDUMP_MAGIC,
    .endian = isbigendian() ? KLCODEDUMP_BIG_ENDIAN : KLCODEDUMP_SMALL_ENDIAN,
    .intsize = sizeof (KlCInt),
    .version = KLCODEDUMP_VERSION,
    .floatsize = sizeof (KlCFloat),
    .unsignedsize = sizeof (unsigned),
  };
  memset(header.reserved, 0, sizeof (header.reserved));
  size_t writesize = ko_write(state->file, &header, sizeof (KlCodeGlobalHeader));
  if (kl_unlikely(writesize != sizeof (KlCodeGlobalHeader)))
    klcode_dump_failure(state);
}

static void klcode_dump_codeheader(KlCodeDumpState* state, KlCodeHeader* header) {
  if (kl_unlikely(sizeof (KlCodeHeader) != ko_write(state->file, header, sizeof (KlCodeHeader))))
    klcode_dump_failure(state);
}

static void klcode_dump_code(KlCodeDumpState* state, KlCode* code) {
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
  klcode_dump_refinfo(state, code->refinfo, header.nref);
  klcode_dump_constants(state, code->strtbl, code->constants, header.nconst);
  klcode_dump_instructions(state, code->code, header.codelen);
  if (header.hasposinfo)
    klcode_dump_posinfo(state, code->posinfo, header.codelen);
  for (size_t i = 0; i < code->nnested; ++i)
    klcode_dump_code(state, code->nestedfunc[i]);
}

static void klcode_dump_refinfo(KlCodeDumpState* state, KlCRefInfo* refinfo, size_t nref) {
  size_t writesize = ko_write(state->file, refinfo, nref * sizeof (KlCRefInfo));
  if (kl_unlikely(writesize != nref * sizeof (KlCRefInfo)))
    klcode_dump_failure(state);
}

static void klcode_dumpinteger(KlCodeDumpState* state, KlCInt val) {
  ko_putc(state->file, KLC_INT);    /* type tag */
  klcode_dumpobj(state, &val, sizeof (val));
}

static void klcode_dumpfloat(KlCodeDumpState* state, KlCFloat val) {
  ko_putc(state->file, KLC_FLOAT);  /* type tag */
  klcode_dumpobj(state, &val, sizeof (val));
}

static void klcode_dumpbool(KlCodeDumpState* state, KlCBool val) {
  ko_putc(state->file, KLC_BOOL);   /* type tag */
  klcode_dumpobj(state, &val, sizeof (val));
}

static void klcode_dumpnil(KlCodeDumpState* state) {
  ko_putc(state->file, KLC_NIL);    /* type tag */
}

static void klcode_dumpstring(KlCodeDumpState* state, KlStrTbl* strtbl, KlStrDesc str) {
  Ko* file = state->file;
  ko_putc(file, KLC_STRING); /* type tag */
  ko_write(file, &(unsigned) { str.length }, sizeof (unsigned));
  ko_write(file, klstrtbl_getstring(strtbl, str.id), str.length * sizeof (char));
}

static void klcode_dump_constants(KlCodeDumpState* state, KlStrTbl* strtbl, KlConstant* constants, size_t nconst) {
  for (size_t i = 0; i < nconst; ++i) {
    switch (constants[i].type) {
      case KLC_INT: {
        klcode_dumpinteger(state, constants[i].intval);
        break;
      }
      case KLC_FLOAT: {
        klcode_dumpfloat(state, constants[i].floatval);
        break;
      }
      case KLC_BOOL: {
        klcode_dumpbool(state, constants[i].boolval);
        break;
      }
      case KLC_NIL: {
        klcode_dumpnil(state);
        break;
      }
      case KLC_STRING: {
        klcode_dumpstring(state, strtbl, constants[i].string);
        break;
      }
      default: {
        kl_assert(false, "unreachable");
        break;
      }
    }
  }
}

static void klcode_dump_instructions(KlCodeDumpState* state, KlInstruction* inst, size_t codelen) {
  size_t writesize = ko_write(state->file, inst, codelen * sizeof (KlInstruction));
  if (kl_unlikely(writesize != codelen * sizeof (KlInstruction)))
    klcode_dump_failure(state);
}

static void klcode_dump_posinfo(KlCodeDumpState* state, KlFilePosition* posinfo, size_t npos) {
  kl_assert(posinfo, "");
  size_t writesize = ko_write(state->file, posinfo, npos * sizeof (KlFilePosition));
  if (kl_unlikely(writesize != npos * sizeof (KlFilePosition)))
    klcode_dump_failure(state);
}

kl_noreturn static void klcode_dump_failure(KlCodeDumpState* state) {
  longjmp(state->jmppos, 1);
}


/* undump */
typedef struct tagKlCodeUndumpState KlCodeUndumpState;
struct tagKlCodeUndumpState {
  Ki* file;
  jmp_buf jmppos;
};

kl_noreturn static void klcode_undump_failure(KlCodeUndumpState* state);
static void klcode_undump_globalheader(KlCodeUndumpState* state, KlCodeGlobalHeader* header);


KlCode* klcode_undump(Ki* file) {
}

static void klcode_undump_globalheader(KlCodeUndumpState* state, KlCodeGlobalHeader* header) {
  if (kl_unlikely(ki_read(state->file, &header, sizeof (KlCodeGlobalHeader)) != sizeof (KlCodeGlobalHeader)))
    klcode_undump_failure(state);
}

kl_noreturn static void klcode_undump_failure(KlCodeUndumpState* state) {
  longjmp(state->jmppos, 1);
}

