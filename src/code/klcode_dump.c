#include "include/code/klcode.h"
#include "include/ast/klast.h"
#include "include/ast/klstrtbl.h"
#include "include/common/klinst.h"
#include "include/misc/klutils.h"
#include <setjmp.h>

#define KLCODEDUMP_MAGIC_SZ         (8)
#define KLCODEDUMP_MAGIC            "\aklang"
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
static void klcode_dump_code(KlCodeDumpState* state, const KlCode* code);
static void klcode_dumpobj(KlCodeDumpState* state, const void* obj, size_t size);
static void klcode_dump_globalheader(KlCodeDumpState* state);
static void klcode_dump_codeheader(KlCodeDumpState* state, const KlCodeHeader* header);
static void klcode_dump_refinfo(KlCodeDumpState* state, const KlCRefInfo* refinfo, size_t nref);
static void klcode_dumpinteger(KlCodeDumpState* state, KlCInt val);
static void klcode_dumpfloat(KlCodeDumpState* state, KlCFloat val);
static void klcode_dumpbool(KlCodeDumpState* state, KlCBool val);
static void klcode_dumpnil(KlCodeDumpState* state);
static void klcode_dumpstring(KlCodeDumpState* state, const KlStrTbl* strtbl, KlStrDesc str);
static void klcode_dump_constants(KlCodeDumpState* state, const KlStrTbl* strtbl, const KlConstant* constants, size_t nconst);
static void klcode_dump_instructions(KlCodeDumpState* state, const KlInstruction* inst, size_t codelen);
static void klcode_dump_posinfo(KlCodeDumpState* state, KlFilePosition* posinfo, size_t npos);


bool klcode_dump(const KlCode* code, Ko* file) {
  KlCodeDumpState dumpstate = {
    .file = file,
  };
  if (setjmp(dumpstate.jmppos) == 0) {
    klcode_dump_globalheader(&dumpstate);
    klcode_dumpstring(&dumpstate, code->strtbl, code->srcfile);
    klcode_dump_code(&dumpstate, code);
    return true;
  } else {
    return false;
  }
}

static void klcode_dumpobj(KlCodeDumpState* state, const void* obj, size_t size) {
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

static void klcode_dump_globalheader(KlCodeDumpState* state) {
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

static void klcode_dump_codeheader(KlCodeDumpState* state, const KlCodeHeader* header) {
  if (kl_unlikely(sizeof (KlCodeHeader) != ko_write(state->file, header, sizeof (KlCodeHeader))))
    klcode_dump_failure(state);
}

static void klcode_dump_code(KlCodeDumpState* state, const KlCode* code) {
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

static void klcode_dump_refinfo(KlCodeDumpState* state, const KlCRefInfo* refinfo, size_t nref) {
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

static void klcode_dumpstring(KlCodeDumpState* state, const KlStrTbl* strtbl, KlStrDesc str) {
  Ko* file = state->file;
  ko_putc(file, KLC_STRING); /* type tag */
  ko_write(file, &(unsigned) { str.length }, sizeof (unsigned));
  ko_write(file, klstrtbl_getstring(strtbl, str.id), str.length * sizeof (char));
}

static void klcode_dump_constants(KlCodeDumpState* state, const KlStrTbl* strtbl, const KlConstant* constants, size_t nconst) {
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

static void klcode_dump_instructions(KlCodeDumpState* state, const KlInstruction* inst, size_t codelen) {
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
  KlStrTbl* strtbl;
  KlStrDesc srcfile;
  KlUnDumpError error;
};

static KlUnDumpError klcode_undump_globalheader(KlCodeUndumpState* state);
static KlCode* klcode_undump_code(KlCodeUndumpState* state);
static KlUnDumpError klcode_undumpobj(KlCodeUndumpState* state, void* buf, size_t size);
static KlUnDumpError klcode_undumpinteger(KlCodeUndumpState* state, KlCInt* buf);
static KlUnDumpError klcode_undumpfloat(KlCodeUndumpState* state, KlCFloat* buf);
static KlUnDumpError klcode_undumpbool(KlCodeUndumpState* state, KlCBool* buf);
static KlUnDumpError klcode_undumpstring_withtypetag(KlCodeUndumpState* state, KlStrDesc* strdesc);
static KlUnDumpError klcode_undumpstring(KlCodeUndumpState* state, KlStrDesc* strdesc);
static KlUnDumpError klcode_undump_refinfo(KlCodeUndumpState* state, KlCRefInfo* refinfo, size_t nref);
static KlUnDumpError klcode_undump_constants(KlCodeUndumpState* state, KlConstant* constants, size_t nconst);
static KlUnDumpError klcode_undump_instructions(KlCodeUndumpState* state, KlInstruction* insts, size_t codelen);
static KlUnDumpError klcode_undump_posinfo(KlCodeUndumpState* state, KlFilePosition* posinfo, size_t codelen);


KlCode* klcode_undump(Ki* file, KlStrTbl* strtbl, KlUnDumpError* perror) {
  KlCodeUndumpState state = {
    .error = KLUNDUMP_SUCCESS,
    .strtbl = strtbl,
    .file = file,
  };
  KlUnDumpError error = klcode_undump_globalheader(&state);
  if (kl_unlikely(error)) {
    if (perror) *perror = error;
    return NULL;
  }
  error = klcode_undumpstring_withtypetag(&state, &state.srcfile);
  if (kl_unlikely(error)) {
    if (perror) *perror = error;
    return NULL;
  }
  KlCode* code = klcode_undump_code(&state);
  if (kl_unlikely(!code)) {
    if (perror) *perror = state.error;
    return NULL;
  }
  if (perror) *perror = KLUNDUMP_SUCCESS;
  return code;
} 

static KlUnDumpError klcode_undump_globalheader(KlCodeUndumpState* state) {
  KlCodeGlobalHeader header;
  if (kl_unlikely(ki_read(state->file, &header, sizeof (KlCodeGlobalHeader)) != sizeof (KlCodeGlobalHeader)))
    return KLUNDUMP_ERROR_BAD;
  if (strncmp(KLCODEDUMP_MAGIC, header.magic, KLCODEDUMP_MAGIC_SZ) != 0)
    return KLUNDUMP_ERROR_MAGIC;
  if (header.endian != (isbigendian() ? KLCODEDUMP_BIG_ENDIAN : KLCODEDUMP_SMALL_ENDIAN))
    return KLUNDUMP_ERROR_ENDIAN;
  if (header.version != KLCODEDUMP_VERSION)
    return KLUNDUMP_ERROR_VERSION;
  if (header.unsignedsize != sizeof (unsigned))
    return KLUNDUMP_ERROR_SIZE;
  if (header.intsize != sizeof (KlCInt))
    return KLUNDUMP_ERROR_SIZE;
  if (header.floatsize != sizeof (KlCFloat))
    return KLUNDUMP_ERROR_SIZE;
  return KLUNDUMP_SUCCESS;
}

static KlCode* klcode_undump_code(KlCodeUndumpState* state) {
  KlCodeHeader header;
  if (kl_unlikely(ki_read(state->file, &header, sizeof (KlCodeHeader)) != sizeof (KlCodeHeader)))
    return NULL;
  KlCRefInfo* refinfo = (KlCRefInfo*)malloc(header.nref * sizeof (KlCRefInfo));
  KlConstant* constants = (KlConstant*)malloc(header.nconst * sizeof (KlConstant));
  KlCode** nestedfunc = (KlCode**)malloc(header.nnested * sizeof (KlCode*));
  KlInstruction* insts = (KlInstruction*)malloc(header.codelen * sizeof (KlInstruction));
  KlFilePosition* posinfo = header.hasposinfo ? (KlFilePosition*)malloc(header.codelen * sizeof (KlFilePosition))
                                              : NULL;
  if (kl_unlikely(!refinfo || !constants || !nestedfunc || !insts || (header.hasposinfo && !posinfo))) {
    free(refinfo);
    free(constants);
    free(nestedfunc);
    free(insts);
    free(posinfo);
    state->error = KLUNDUMP_ERROR_OOM;
    return NULL;
  }
  KlUnDumpError error = klcode_undump_refinfo(state, refinfo, header.nref);
  if (kl_likely(error == KLUNDUMP_SUCCESS))
    error = klcode_undump_constants(state, constants, header.nconst);
  if (kl_likely(error == KLUNDUMP_SUCCESS))
    error = klcode_undump_instructions(state, insts, header.codelen);
  if (kl_likely(error == KLUNDUMP_SUCCESS)) {
    if (header.hasposinfo)
      error = klcode_undump_posinfo(state, posinfo, header.codelen);
  }
  if (kl_likely(error == KLUNDUMP_SUCCESS)) {
    error = KLUNDUMP_SUCCESS;
    for (size_t i = 0; i < header.nnested; ++i) {
      KlCode* nested = klcode_undump_code(state);
      if (kl_unlikely(!nested)) {
        error = state->error;
        for (size_t j = 0; j < i; ++j)
          klcode_delete(nestedfunc[j]);
        break;
      }
      nestedfunc[i] = nested;
    }
  }
  if (kl_unlikely(error)) {
    free(refinfo);
    free(constants);
    free(nestedfunc);
    free(insts);
    free(posinfo);
    state->error = error;
    return NULL;
  }
  KlCode* code = klcode_create(refinfo, header.nref, constants, header.nconst,
                               insts, posinfo, header.codelen, nestedfunc,
                               header.nnested, state->strtbl, header.nparam,
                               header.framesize, state->srcfile);
  if (kl_unlikely(!code)) {
    state->error = KLUNDUMP_ERROR_OOM;
    return NULL;
  }
  return code;
}

static KlUnDumpError klcode_undump_refinfo(KlCodeUndumpState* state, KlCRefInfo* refinfo, size_t nref) {
  if (kl_unlikely(ki_read(state->file, refinfo, nref * sizeof (KlCRefInfo)) != nref * sizeof (KlCRefInfo)))
    return KLUNDUMP_ERROR_BAD;
  return KLUNDUMP_SUCCESS;
}

static KlUnDumpError klcode_undumpobj(KlCodeUndumpState* state, void* buf, size_t size) {
  if (kl_unlikely(ki_read(state->file, buf, size) != size))
    return KLUNDUMP_ERROR_BAD;
  return KLUNDUMP_SUCCESS;
}

static KlUnDumpError klcode_undumpinteger(KlCodeUndumpState* state, KlCInt* buf) {
  return klcode_undumpobj(state, buf, sizeof (KlCInt));
}

static KlUnDumpError klcode_undumpfloat(KlCodeUndumpState* state, KlCFloat* buf) {
  return klcode_undumpobj(state, buf, sizeof (KlCFloat));
}

static KlUnDumpError klcode_undumpbool(KlCodeUndumpState* state, KlCBool* buf) {
  return klcode_undumpobj(state, buf, sizeof (KlCBool));
}

static KlUnDumpError klcode_undumpstring_withtypetag(KlCodeUndumpState* state, KlStrDesc* strdesc) {
  int type = ki_getc(state->file);
  if (kl_unlikely(type == KOF || type != KLC_STRING))
    return KLUNDUMP_ERROR_BAD;
  return klcode_undumpstring(state, strdesc);
}

static KlUnDumpError klcode_undumpstring(KlCodeUndumpState* state, KlStrDesc* strdesc) {
  unsigned len;
  if (kl_unlikely(ki_read(state->file, &len, sizeof (unsigned)) != sizeof (unsigned)))
    return KLUNDUMP_ERROR_BAD;
  char* buf = klstrtbl_allocstring(state->strtbl, len);
  if (kl_unlikely(!buf)) return KLUNDUMP_ERROR_OOM;
  if (kl_unlikely(ki_read(state->file, buf, len * sizeof (char)) != len * sizeof (char)))
    return KLUNDUMP_ERROR_BAD;
  strdesc->id = klstrtbl_pushstring(state->strtbl, len);
  strdesc->length = len;
  return KLUNDUMP_SUCCESS;
}


static KlUnDumpError klcode_undump_constants(KlCodeUndumpState* state, KlConstant* constants, size_t nconst) {
  for (size_t i = 0; i < nconst; ++i) {
    int type = ki_getc(state->file);
    if (type == KOF) return KLUNDUMP_ERROR_BAD;
    constants[i].type = type;
    KlUnDumpError error = KLUNDUMP_SUCCESS;
    switch (type) {
      case KLC_INT: {
        error = klcode_undumpinteger(state, &constants[i].intval);
        break;
      }
      case KLC_FLOAT: {
        error = klcode_undumpfloat(state, &constants[i].floatval);
        break;
      }
      case KLC_BOOL: {
        error = klcode_undumpbool(state, &constants[i].boolval);
        break;
      }
      case KLC_NIL: {
        break;
      }
      case KLC_STRING: {
        error = klcode_undumpstring(state, &constants[i].string);
        break;
      }
      default: {
        return KLUNDUMP_ERROR_BAD;
      }
    }
    if (kl_unlikely(error)) return error;
  }
  return KLUNDUMP_SUCCESS;
}

static KlUnDumpError klcode_undump_instructions(KlCodeUndumpState* state, KlInstruction* insts, size_t codelen) {
  if (kl_unlikely(ki_read(state->file, insts, codelen * sizeof (KlInstruction)) != codelen * sizeof (KlInstruction)))
    return KLUNDUMP_ERROR_BAD;
  return KLUNDUMP_SUCCESS;
}

static KlUnDumpError klcode_undump_posinfo(KlCodeUndumpState* state, KlFilePosition* posinfo, size_t codelen) {
  if (kl_unlikely(ki_read(state->file, posinfo, codelen * sizeof (KlFilePosition)) != codelen * sizeof (KlFilePosition)))
    return KLUNDUMP_ERROR_BAD;
  return KLUNDUMP_SUCCESS;
}
