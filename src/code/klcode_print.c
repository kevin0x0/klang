#include "include/code/klcode.h"
#include "include/misc/klutils.h"

static void print_prefix(const KlCode* code, Ko* out, KlInstruction* pc, const char* name) {
  code->posinfo ? ko_printf(out, "(%4u, %4u) [%03u] %-15s", code->posinfo[pc - code->code].begin, code->posinfo[pc - code->code].end, pc - code->code, name)
                 : ko_printf(out, "(no position info) [%03u] %-15s", pc - code->code, name);
}

static const char* get_instname(KlInstruction inst) {
  static const char* const names[KLOPCODE_NINST] = {
    [KLOPCODE_MOVE] = "MOVE",
    [KLOPCODE_MULTIMOVE] = "MULTIMOVE",
    [KLOPCODE_ADD] = "ADD",
    [KLOPCODE_SUB] = "SUB",
    [KLOPCODE_MUL] = "MUL",
    [KLOPCODE_DIV] = "DIV",
    [KLOPCODE_MOD] = "MOD",
    [KLOPCODE_IDIV] = "IDIV",
    [KLOPCODE_CONCAT] = "CONCAT",
    [KLOPCODE_ADDI] = "ADDI",
    [KLOPCODE_SUBI] = "SUBI",
    [KLOPCODE_MULI] = "MULI",
    [KLOPCODE_DIVI] = "DIVI",
    [KLOPCODE_MODI] = "MODI",
    [KLOPCODE_IDIVI] = "IDIVI",
    [KLOPCODE_ADDC] = "ADDC",
    [KLOPCODE_SUBC] = "SUBC",
    [KLOPCODE_MULC] = "MULC",
    [KLOPCODE_DIVC] = "DIVC",
    [KLOPCODE_MODC] = "MODC",
    [KLOPCODE_IDIVC] = "IDIVC",
    [KLOPCODE_LEN] = "LEN",
    [KLOPCODE_NEG] = "NEG",
    [KLOPCODE_SCALL] = "SCALL",
    [KLOPCODE_CALL] = "CALL",
    [KLOPCODE_METHOD] = "METHOD",
    [KLOPCODE_RETURN] = "RETURN",
    [KLOPCODE_RETURN0] = "RETURN0",
    [KLOPCODE_RETURN1] = "RETURN1",
    [KLOPCODE_LOADBOOL] = "LOADBOOL",
    [KLOPCODE_LOADI] = "LOADI",
    [KLOPCODE_LOADC] = "LOADC",
    [KLOPCODE_LOADNIL] = "LOADNIL",
    [KLOPCODE_LOADREF] = "LOADREF",
    [KLOPCODE_LOADGLOBAL] = "LOADGLOBAL",
    [KLOPCODE_STOREREF] = "STOREREF",
    [KLOPCODE_STOREGLOBAL] = "STOREGLOBAL",
    [KLOPCODE_MKTUPLE] = "MKTUPLE",
    [KLOPCODE_MKMAP] = "MKMAP",
    [KLOPCODE_MKARRAY] = "MKARRAY",
    [KLOPCODE_MKCLOSURE] = "MKCLOSURE",
    [KLOPCODE_APPEND] = "APPEND",
    [KLOPCODE_MKCLASS] = "MKCLASS",
    [KLOPCODE_INDEXI] = "INDEXI",
    [KLOPCODE_INDEXASI] = "INDEXASI",
    [KLOPCODE_INDEX] = "INDEX",
    [KLOPCODE_INDEXAS] = "INDEXAS",
    [KLOPCODE_GETFIELDR] = "GETFIELDR",
    [KLOPCODE_GETFIELDC] = "GETFIELDC",
    [KLOPCODE_SETFIELDR] = "SETFIELDR",
    [KLOPCODE_SETFIELDC] = "SETFIELDC",
    [KLOPCODE_REFGETFIELDR] = "REFGETFIELDR",
    [KLOPCODE_REFGETFIELDC] = "REFGETFIELDC",
    [KLOPCODE_REFSETFIELDR] = "REFSETFIELDR",
    [KLOPCODE_REFSETFIELDC] = "REFSETFIELDC",
    [KLOPCODE_NEWLOCAL] = "NEWLOCAL",
    [KLOPCODE_NEWMETHODC] = "NEWMETHODC",
    [KLOPCODE_NEWMETHODR] = "NEWMETHODR",
    [KLOPCODE_LOADFALSESKIP] = "LOADFALSESKIP",
    [KLOPCODE_TESTSET] = "TESTSET",
    [KLOPCODE_TRUEJMP] = "TRUEJMP",
    [KLOPCODE_FALSEJMP] = "FALSEJMP",
    [KLOPCODE_JMP] = "JMP",
    [KLOPCODE_CONDJMP] = "CONDJMP",
    [KLOPCODE_CLOSEJMP] = "CLOSEJMP",
    [KLOPCODE_HASFIELD] = "HASFIELD",
    [KLOPCODE_IS] = "IS",
    [KLOPCODE_EQ] = "EQ",
    [KLOPCODE_NE] = "NE",
    [KLOPCODE_LE] = "LE",
    [KLOPCODE_GE] = "GE",
    [KLOPCODE_LT] = "LT",
    [KLOPCODE_GT] = "GT",
    [KLOPCODE_EQC] = "EQC",
    [KLOPCODE_NEC] = "NEC",
    [KLOPCODE_LEC] = "LEC",
    [KLOPCODE_GEC] = "GEC",
    [KLOPCODE_LTC] = "LTC",
    [KLOPCODE_GTC] = "GTC",
    [KLOPCODE_EQI] = "EQI",
    [KLOPCODE_NEI] = "NEI",
    [KLOPCODE_LEI] = "LEI",
    [KLOPCODE_GEI] = "GEI",
    [KLOPCODE_LTI] = "LTI",
    [KLOPCODE_GTI] = "GTI",
    [KLOPCODE_MATCH] = "MATCH",
    [KLOPCODE_PBARR] = "PBARR",
    [KLOPCODE_PBTUP] = "PBTUP",
    [KLOPCODE_PBMAP] = "PBMAP",
    [KLOPCODE_PBOBJ] = "PBOBJ",
    [KLOPCODE_PMARR] = "PMARR",
    [KLOPCODE_PMTUP] = "PMTUP",
    [KLOPCODE_PMMAP] = "PMMAP",
    [KLOPCODE_PMOBJ] = "PMOBJ",
    [KLOPCODE_PMAPPOST] = "PMAPPOST",
    [KLOPCODE_NEWOBJ] = "NEWOBJ",
    [KLOPCODE_ADJUSTARGS] = "ADJUSTARGS",
    [KLOPCODE_VFORPREP] = "VFORPREP",
    [KLOPCODE_VFORLOOP] = "VFORLOOP",
    [KLOPCODE_IFORPREP] = "IFORPREP",
    [KLOPCODE_IFORLOOP] = "IFORLOOP",
    [KLOPCODE_GFORPREP] = "GFORPREP",
    [KLOPCODE_GFORLOOP] = "GFORLOOP",
    [KLOPCODE_ASYNC] = "ASYNC",
    [KLOPCODE_YIELD] = "YIELD",
    [KLOPCODE_VARARG] = "VARARG",
  };
  return names[KLINST_GET_OPCODE(inst)];
}

static void print_(Ko* out, KlInstruction inst) {
  kl_unused(inst);
  ko_printf(out, "| %4s %4s %4s %4s | ", "", "", "", "");
}

static void print_A(Ko* out, KlInstruction inst) {
  ko_printf(out, "| %4u %4s %4s %4s | ", (unsigned)KLINST_A_GETA(inst), "", "", "");
}

static void print_AB(Ko* out, KlInstruction inst) {
  ko_printf(out, "| %4u %4u %4s %4s | ", (unsigned)KLINST_ABC_GETA(inst), (unsigned)KLINST_ABC_GETB(inst), "", "");
}

static void print_AX(Ko* out, KlInstruction inst) {
  ko_printf(out, "| %4u %4u %4s %4s | ", (unsigned)KLINST_AX_GETA(inst), (unsigned)KLINST_AX_GETX(inst), "", "");
}

static void print_ABC(Ko* out, KlInstruction inst) {
  ko_printf(out, "| %4u %4u %4u %4s | ", (unsigned)KLINST_ABC_GETA(inst), (unsigned)KLINST_ABC_GETB(inst), (unsigned)KLINST_ABC_GETC(inst), "");
}

static void print_ABX(Ko* out, KlInstruction inst) {
  ko_printf(out, "| %4u %4u %4u %4s | ", (unsigned)KLINST_ABX_GETA(inst), (unsigned)KLINST_ABX_GETB(inst), (unsigned)KLINST_ABX_GETX(inst), "");
}

static void print_ABTX(Ko* out, KlInstruction inst) {
  ko_printf(out, "| %4u %4u %4u %4u | ", (unsigned)KLINST_ABTX_GETA(inst), (unsigned)KLINST_ABTX_GETB(inst), (unsigned)(KLINST_ABTX_GETT(inst) != 0), (unsigned)KLINST_ABTX_GETX(inst));
}

static void print_AXY(Ko* out, KlInstruction inst) {
  ko_printf(out, "| %4u %4u %4u %4s | ", (unsigned)KLINST_AXY_GETA(inst), (unsigned)KLINST_AXY_GETX(inst), (unsigned)KLINST_AXY_GETY(inst), "");
}

static void print_XYZ(Ko* out, KlInstruction inst) {
  ko_printf(out, "| %4u %4u %4u %4s | ", (unsigned)KLINST_XYZ_GETX(inst), (unsigned)KLINST_XYZ_GETY(inst), (unsigned)KLINST_XYZ_GETZ(inst), "");
}

static void print_ABI(Ko* out, KlInstruction inst) {
  ko_printf(out, "| %4u %4u %4d %4s | ", (unsigned)KLINST_ABI_GETA(inst), (unsigned)KLINST_ABI_GETB(inst), (signed)KLINST_ABI_GETI(inst), "");
}

static void print_AI(Ko* out, KlInstruction inst) {
  ko_printf(out, "| %4u %4d %4s %4s | ", (unsigned)KLINST_AI_GETA(inst), (signed)KLINST_AI_GETI(inst), "", "");
}

static void print_XI(Ko* out, KlInstruction inst) {
  ko_printf(out, "| %4u %4d %4s %4s | ", (unsigned)KLINST_XI_GETX(inst), (signed)KLINST_AI_GETI(inst), "", "");
}

static void print_I(Ko* out, KlInstruction inst) {
  ko_printf(out, "| %4d %4s %4s %4s | ", (signed)KLINST_I_GETI(inst), "", "", "");
}

static void print_constant(const KlCode* code, Ko* out, KlConstant* constant) {
  switch (constant->type) {
    case KLC_INT: {
      ko_printf(out, "%zd", constant->intval);
      break;
    }
    case KLC_FLOAT: {
      ko_printf(out, "%#lf", constant->floatval);
      break;
    }
    case KLC_STRING: {
      ko_printf(out, "\"%.*s\"", constant->string.length, klstrtbl_getstring(code->strtbl, constant->string.id));
      break;
    }
    case KLC_BOOL: {
      ko_printf(out, "%s", constant->boolval ? "true" : "false");
      break;
    }
    case KLC_NIL: {
      ko_printf(out, "nil");
      break;
    }
    default: {
      kl_assert(false, "impossible constant type");
    }
  }
}

static void print_string_noquote(const KlCode* code, Ko* out, KlConstant* constant) {
  kl_assert(constant->type == KLC_STRING, "");
  ko_printf(out, "%.*s", constant->string.length, klstrtbl_getstring(code->strtbl, constant->string.id));
}

static KlInstruction* print_instruction(const KlCode* code, Ko* out, KlInstruction* pc) {
  KlInstruction inst = *pc++;
  print_prefix(code, out, pc - 1, get_instname(inst));
  uint8_t opcode = KLINST_GET_OPCODE(inst);
  switch (opcode) {
    case KLOPCODE_MOVE: {
      print_AB(out, inst);
      return pc;
    }
    case KLOPCODE_MULTIMOVE: {
      print_ABX(out, inst);
      size_t nmove = KLINST_ABX_GETX(inst);
      nmove == KLINST_VARRES ? ko_printf(out, "move all") : ko_printf(out, "move %u values", nmove);
      return pc;
    }
    case KLOPCODE_ADD: {
      print_ABC(out, inst);
      return pc;
    }
    case KLOPCODE_SUB: {
      print_ABC(out, inst);
      return pc;
    }
    case KLOPCODE_MUL: {
      print_ABC(out, inst);
      return pc;
    }
    case KLOPCODE_DIV: {
      print_ABC(out, inst);
      return pc;
    }
    case KLOPCODE_MOD: {
      print_ABC(out, inst);
      return pc;
    }
    case KLOPCODE_IDIV: {
      print_ABC(out, inst);
      return pc;
    }
    case KLOPCODE_CONCAT: {
      print_ABC(out, inst);
      return pc;
    }
    case KLOPCODE_ADDI: {
      print_ABI(out, inst);
      ko_printf(out, "%d", KLINST_ABI_GETI(inst));
      return pc;
    }
    case KLOPCODE_SUBI: {
      print_ABI(out, inst);
      ko_printf(out, "%d", KLINST_ABI_GETI(inst));
      return pc;
    }
    case KLOPCODE_MULI: {
      print_ABI(out, inst);
      ko_printf(out, "%d", KLINST_ABI_GETI(inst));
      return pc;
    }
    case KLOPCODE_DIVI: {
      print_ABI(out, inst);
      ko_printf(out, "%d", KLINST_ABI_GETI(inst));
      return pc;
    }
    case KLOPCODE_MODI: {
      print_ABI(out, inst);
      ko_printf(out, "%d", KLINST_ABI_GETI(inst));
      return pc;
    }
    case KLOPCODE_IDIVI: {
      print_ABI(out, inst);
      ko_printf(out, "%d", KLINST_ABI_GETI(inst));
      return pc;
    }
    case KLOPCODE_ADDC: {
      print_ABX(out, inst);
      print_constant(code, out, &code->constants[KLINST_ABX_GETX(inst)]);
      return pc;
    }
    case KLOPCODE_SUBC: {
      print_ABX(out, inst);
      print_constant(code, out, &code->constants[KLINST_ABX_GETX(inst)]);
      return pc;
    }
    case KLOPCODE_MULC: {
      print_ABX(out, inst);
      print_constant(code, out, &code->constants[KLINST_ABX_GETX(inst)]);
      return pc;
    }
    case KLOPCODE_DIVC: {
      print_ABX(out, inst);
      print_constant(code, out, &code->constants[KLINST_ABX_GETX(inst)]);
      return pc;
    }
    case KLOPCODE_MODC: {
      print_ABX(out, inst);
      print_constant(code, out, &code->constants[KLINST_ABX_GETX(inst)]);
      return pc;
    }
    case KLOPCODE_IDIVC: {
      print_ABX(out, inst);
      print_constant(code, out, &code->constants[KLINST_ABX_GETX(inst)]);
      return pc;
    }
    case KLOPCODE_LEN: {
      print_AB(out, inst);
      return pc;
    }
    case KLOPCODE_NEG: {
      print_AB(out, inst);
      return pc;
    }
    case KLOPCODE_SCALL: {
      print_AXY(out, inst);
      size_t narg = KLINST_AXY_GETX(inst);
      size_t nret = KLINST_AXY_GETY(inst);
      narg == KLINST_VARRES ? ko_printf(out, "all arguments, ") : ko_printf(out, "%u arguments, ", narg);
      nret == KLINST_VARRES ? ko_printf(out, "all results") : ko_printf(out, "%u results", nret);
      return pc;
    }
    case KLOPCODE_CALL: {
      print_A(out, inst);
      ko_printf(out, "callable object at R%u\n", KLINST_AXY_GETA(inst));
      KlInstruction extra = *pc++;

      print_prefix(code, out, pc - 1, "CALL EXTRA");
      print_XYZ(out, extra);
      size_t narg = KLINST_XYZ_GETX(extra);
      size_t nret = KLINST_XYZ_GETY(extra);
      size_t target = KLINST_XYZ_GETZ(extra);
      narg == KLINST_VARRES ? ko_printf(out, "all arguments, ") : ko_printf(out, "%u arguments, ", narg);
      nret == KLINST_VARRES ? ko_printf(out, "all results, ") : ko_printf(out, "%u results, ", nret);
      ko_printf(out, "target at R%u", target);
      return pc;
    }
    case KLOPCODE_METHOD: {
      print_AX(out, inst);
      ko_printf(out, "object at R%u, method name: ", KLINST_AX_GETA(inst));
      print_string_noquote(code, out, &code->constants[KLINST_AX_GETX(inst)]);
      ko_putc(out, '\n');
      KlInstruction extra = *pc++;

      print_prefix(code, out, pc - 1, "METHOD EXTRA");
      print_XYZ(out, extra);
      size_t narg = KLINST_XYZ_GETX(extra);
      size_t nret = KLINST_XYZ_GETY(extra);
      size_t target = KLINST_XYZ_GETZ(extra);
      narg == KLINST_VARRES ? ko_printf(out, "all arguments, ") : ko_printf(out, "%u arguments, ", narg);
      nret == KLINST_VARRES ? ko_printf(out, "all results, ") : ko_printf(out, "%u results, ", nret);
      ko_printf(out, "target at R%u", target);
      return pc;
    }
    case KLOPCODE_RETURN: {
      size_t nres = KLINST_AX_GETX(inst);
      print_AX(out, inst);
      nres == KLINST_VARRES ? ko_printf(out, "all results") : ko_printf(out, "%u results", nres);
      return pc;
    }
    case KLOPCODE_RETURN0: {
      print_(out, inst);
      return pc;
    }
    case KLOPCODE_RETURN1: {
      print_A(out, inst);
      return pc;
    }
    case KLOPCODE_LOADBOOL: {
      print_AX(out, inst);
      ko_printf(out, KLINST_AX_GETX(inst) ? "true" : "false");
      return pc;
    }
    case KLOPCODE_LOADI: {
      print_AI(out, inst);
      ko_printf(out, "%d", KLINST_AI_GETI(inst));
      return pc;
    }
    case KLOPCODE_LOADC: {
      print_AX(out, inst);
      print_constant(code, out, &code->constants[KLINST_AX_GETX(inst)]);
      return pc;
    }
    case KLOPCODE_LOADNIL: {
      print_AX(out, inst);
      ko_printf(out, "%u nils", KLINST_AX_GETX(inst));
      return pc;
    }
    case KLOPCODE_LOADREF: {
      print_AX(out, inst);
      return pc;
    }
    case KLOPCODE_LOADGLOBAL: {
      print_AX(out, inst);
      print_string_noquote(code, out, &code->constants[KLINST_AX_GETX(inst)]);
      return pc;
    }
    case KLOPCODE_STOREREF: {
      print_AX(out, inst);
      return pc;
    }
    case KLOPCODE_STOREGLOBAL: {
      print_AX(out, inst);
      print_string_noquote(code, out, &code->constants[KLINST_AX_GETX(inst)]);
      return pc;
    }
    case KLOPCODE_MKTUPLE: {
      print_ABX(out, inst);
      size_t nexpr = KLINST_ABX_GETX(inst);
      ko_printf(out, "%u values", nexpr);
      return pc;
    }
    case KLOPCODE_MKMAP: {
      /* this instruction tells us current stack top */
      size_t stktop = KLINST_ABX_GETB(inst);
      size_t capacity = KLINST_ABX_GETX(inst);
      print_ABC(out, inst);
      ko_printf(out, "capacity: %u, stack top: %u", klbit(capacity), stktop);
      return pc;
    }
    case KLOPCODE_MKARRAY: {
      print_ABX(out, inst);
      size_t nelem = KLINST_ABX_GETX(inst);
      nelem == KLINST_VARRES ? ko_printf(out, "many elements") : ko_printf(out, "%u elements", nelem);
      return pc;
    }
    case KLOPCODE_MKCLOSURE: {
      print_AX(out, inst);
      return pc;
    }
    case KLOPCODE_APPEND: {
      print_ABX(out, inst);
      size_t nelem = KLINST_ABX_GETX(inst);
      nelem == KLINST_VARRES ? ko_printf(out, "many elements") : ko_printf(out, "%u elements", nelem);
      return pc;
    }
    case KLOPCODE_MKCLASS: {
      /* this instruction tells us current stack top */
      size_t stktop = KLINST_ABTX_GETB(inst);
      size_t capacity = KLINST_ABTX_GETX(inst);
      print_ABTX(out, inst);
      ko_printf(out, "capacity: %u, stack top: %u", klbit(capacity), stktop);
      if (KLINST_ABTX_GETT(inst))
        ko_printf(out, ", parent at R%u", stktop);
      return pc;
    }
    case KLOPCODE_INDEXI: {
      print_ABI(out, inst);
      ko_printf(out, "R%u = R%u[%d]", KLINST_ABI_GETA(inst), KLINST_ABI_GETB(inst), KLINST_ABI_GETI(inst));
      return pc;
    }
    case KLOPCODE_INDEXASI: {
      print_ABI(out, inst);
      ko_printf(out, "R%u[%d] = R%u", KLINST_ABI_GETB(inst), KLINST_ABI_GETI(inst), KLINST_ABI_GETA(inst));
      return pc;
    }
    case KLOPCODE_INDEX: {
      print_ABC(out, inst);
      ko_printf(out, "R%u = R%u[R%d]", KLINST_ABC_GETA(inst), KLINST_ABC_GETB(inst), KLINST_ABC_GETC(inst));
      return pc;
    }
    case KLOPCODE_INDEXAS: {
      print_ABC(out, inst);
      ko_printf(out, "R%u[R%d] = R%u", KLINST_ABC_GETB(inst), KLINST_ABC_GETC(inst), KLINST_ABC_GETA(inst));
      return pc;
    }
    case KLOPCODE_GETFIELDR: {
      print_ABC(out, inst);
      ko_printf(out, "R%u = R%u.R%d", KLINST_ABC_GETA(inst), KLINST_ABC_GETB(inst), KLINST_ABC_GETC(inst));
      return pc;
    }
    case KLOPCODE_GETFIELDC: {
      print_ABC(out, inst);
      ko_printf(out, "R%u = R%u.", KLINST_ABC_GETA(inst), KLINST_ABC_GETB(inst));
      print_string_noquote(code, out, &code->constants[KLINST_ABC_GETC(inst)]);
      return pc;
    }
    case KLOPCODE_SETFIELDR: {
      print_ABC(out, inst);
      ko_printf(out, "R%u.R%u = R%u", KLINST_ABC_GETB(inst), KLINST_ABC_GETC(inst), KLINST_ABC_GETA(inst));
      return pc;
    }
    case KLOPCODE_SETFIELDC: {
      print_ABC(out, inst);
      ko_printf(out, "R%u.", KLINST_ABC_GETB(inst));
      print_string_noquote(code, out, &code->constants[KLINST_ABC_GETC(inst)]);
      ko_printf(out, " = R%u", KLINST_ABC_GETA(inst));
      return pc;
    }
    case KLOPCODE_REFGETFIELDR: {
      print_ABC(out, inst);
      ko_printf(out, "R%u = REF%u.R%u", KLINST_ABX_GETA(inst), KLINST_ABX_GETX(inst), KLINST_ABX_GETB(inst));
      return pc;
    }
    case KLOPCODE_REFGETFIELDC: {
      print_ABC(out, inst);
      ko_printf(out, "R%u = REF%u.", KLINST_AXY_GETA(inst), KLINST_AXY_GETY(inst)); 
      print_string_noquote(code, out, &code->constants[KLINST_AXY_GETX(inst)]);
      return pc;
    }
    case KLOPCODE_REFSETFIELDR: {
      print_ABC(out, inst);
      ko_printf(out, "REF%u.R%u = R%u", KLINST_AXY_GETX(inst), KLINST_AXY_GETY(inst), KLINST_AXY_GETA(inst));
      return pc;
    }
    case KLOPCODE_REFSETFIELDC: {
      print_ABC(out, inst);
      ko_printf(out, "REF%u.", KLINST_AXY_GETY(inst));
      print_string_noquote(code, out, &code->constants[KLINST_AXY_GETX(inst)]);
      ko_printf(out, " = R%u", KLINST_AXY_GETA(inst));
      return pc;
    }
    case KLOPCODE_NEWLOCAL: {
      print_AX(out, inst);
      print_constant(code, out, &code->constants[KLINST_AX_GETX(inst)]);
      return pc;
    }
    case KLOPCODE_NEWMETHODR: {
      print_ABC(out, inst);
      ko_printf(out, "method R%u.R%u = R%u", KLINST_ABC_GETA(inst), KLINST_ABC_GETC(inst), KLINST_ABC_GETB(inst));
      return pc;
    }
    case KLOPCODE_NEWMETHODC: {
      print_ABX(out, inst);
      ko_printf(out, "method R%u.", KLINST_ABC_GETA(inst));
      print_string_noquote(code, out, &code->constants[KLINST_ABC_GETC(inst)]);
      ko_printf(out, " = R%u", KLINST_ABC_GETB(inst));
      return pc;
    }
    case KLOPCODE_LOADFALSESKIP: {
      print_A(out, inst);
      return pc;
    }
    case KLOPCODE_TESTSET: {
      print_AB(out, inst);
      ko_putc(out, '\n');
      kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
      KlInstruction extra = *pc++;
      print_prefix(code, out, pc - 1, get_instname(extra));
      print_XI(out, extra);
      ko_printf(out, "set R%u = R%u and jump to %u if %s", KLINST_ABC_GETA(inst), KLINST_ABC_GETB(inst), pc + KLINST_XI_GETI(extra) - code->code, KLINST_XI_GETX(extra) ? "true" : "false");
      return pc;
    }
    case KLOPCODE_TRUEJMP: {
      print_AI(out, inst);
      ko_printf(out, "jump to %d if true", pc + KLINST_AI_GETI(inst) - code->code);
      return pc;
    }
    case KLOPCODE_FALSEJMP: {
      print_AI(out, inst);
      ko_printf(out, "jump to %u if false", pc + KLINST_AI_GETI(inst) - code->code);
      return pc;
    }
    case KLOPCODE_JMP: {
      print_I(out, inst);
      ko_printf(out, "jump to %u", pc + KLINST_I_GETI(inst) - code->code);
      return pc;
    }
    case KLOPCODE_CONDJMP: {
      print_XI(out, inst);
      ko_printf(out, "this instruction should not be printed");
      return pc;
    }
    case KLOPCODE_CLOSEJMP: {
      print_XI(out, inst);
      ko_printf(out, "close from %u then jump to %u", KLINST_XI_GETX(inst), pc + KLINST_XI_GETI(inst) - code->code);
      return pc;
      break;
    }
    case KLOPCODE_HASFIELD: {
      print_AX(out, inst);
      print_constant(code, out, &code->constants[KLINST_AX_GETX(inst)]);
      ko_putc(out, '\n');
      kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
      KlInstruction extra = *pc++;
      print_prefix(code, out, pc - 1, get_instname(extra));
      print_XI(out, extra);
      ko_printf(out, "jump to %u if %s", pc + KLINST_XI_GETI(extra) - code->code, KLINST_XI_GETX(extra) ? "true" : "false");
      return pc;
    }
    case KLOPCODE_IS: {
      print_AB(out, inst);
      ko_putc(out, '\n');
      kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
      KlInstruction extra = *pc++;
      print_prefix(code, out, pc - 1, get_instname(extra));
      print_XI(out, extra);
      ko_printf(out, "jump to %u if %s", pc + KLINST_XI_GETI(extra) - code->code, KLINST_XI_GETX(extra) ? "true" : "false");
      return pc;
    }
    case KLOPCODE_EQ: {
      print_AB(out, inst);
      ko_putc(out, '\n');
      kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
      KlInstruction extra = *pc++;
      print_prefix(code, out, pc - 1, get_instname(extra));
      print_XI(out, extra);
      ko_printf(out, "jump to %u if %s", pc + KLINST_XI_GETI(extra) - code->code, KLINST_XI_GETX(extra) ? "true" : "false");
      return pc;
    }
    case KLOPCODE_NE: {
      print_AB(out, inst);
      ko_putc(out, '\n');
      kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
      KlInstruction extra = *pc++;
      print_prefix(code, out, pc - 1, get_instname(extra));
      print_XI(out, extra);
      ko_printf(out, "jump to %u if %s", pc + KLINST_XI_GETI(extra) - code->code, KLINST_XI_GETX(extra) ? "true" : "false");
      return pc;
    }
    case KLOPCODE_LT: {
      print_AB(out, inst);
      ko_putc(out, '\n');
      kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
      KlInstruction extra = *pc++;
      print_prefix(code, out, pc - 1, get_instname(extra));
      print_XI(out, extra);
      ko_printf(out, "jump to %u if %s", pc + KLINST_XI_GETI(extra) - code->code, KLINST_XI_GETX(extra) ? "true" : "false");
      return pc;
    }
    case KLOPCODE_GT: {
      print_AB(out, inst);
      ko_putc(out, '\n');
      kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
      KlInstruction extra = *pc++;
      print_prefix(code, out, pc - 1, get_instname(extra));
      print_XI(out, extra);
      ko_printf(out, "jump to %u if %s", pc + KLINST_XI_GETI(extra) - code->code, KLINST_XI_GETX(extra) ? "true" : "false");
      return pc;
    }
    case KLOPCODE_LE: {
      print_AB(out, inst);
      ko_putc(out, '\n');
      kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
      KlInstruction extra = *pc++;
      print_prefix(code, out, pc - 1, get_instname(extra));
      print_XI(out, extra);
      ko_printf(out, "jump to %u if %s", pc + KLINST_XI_GETI(extra) - code->code, KLINST_XI_GETX(extra) ? "true" : "false");
      return pc;
    }
    case KLOPCODE_GE: {
      print_AB(out, inst);
      ko_putc(out, '\n');
      kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
      KlInstruction extra = *pc++;
      print_prefix(code, out, pc - 1, get_instname(extra));
      print_XI(out, extra);
      ko_printf(out, "jump to %u if %s", pc + KLINST_XI_GETI(extra) - code->code, KLINST_XI_GETX(extra) ? "true" : "false");
      return pc;
    }
    case KLOPCODE_EQC: {
      print_AX(out, inst);
      print_constant(code, out, &code->constants[KLINST_AX_GETX(inst)]);
      ko_putc(out, '\n');
      kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
      KlInstruction extra = *pc++;
      print_prefix(code, out, pc - 1, get_instname(extra));
      print_XI(out, extra);
      ko_printf(out, "jump to %u if %s", pc + KLINST_XI_GETI(extra) - code->code, KLINST_XI_GETX(extra) ? "true" : "false");
      return pc;
    }
    case KLOPCODE_NEC: {
      print_AX(out, inst);
      print_constant(code, out, &code->constants[KLINST_AX_GETX(inst)]);
      ko_putc(out, '\n');
      kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
      KlInstruction extra = *pc++;
      print_prefix(code, out, pc - 1, get_instname(extra));
      print_XI(out, extra);
      ko_printf(out, "jump to %u if %s", pc + KLINST_XI_GETI(extra) - code->code, KLINST_XI_GETX(extra) ? "true" : "false");
      return pc;
    }
    case KLOPCODE_LTC: {
      print_AX(out, inst);
      print_constant(code, out, &code->constants[KLINST_AX_GETX(inst)]);
      ko_putc(out, '\n');
      kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
      KlInstruction extra = *pc++;
      print_prefix(code, out, pc - 1, get_instname(extra));
      print_XI(out, extra);
      ko_printf(out, "jump to %u if %s", pc + KLINST_XI_GETI(extra) - code->code, KLINST_XI_GETX(extra) ? "true" : "false");
      return pc;
    }
    case KLOPCODE_GTC: {
      print_AX(out, inst);
      print_constant(code, out, &code->constants[KLINST_AX_GETX(inst)]);
      ko_putc(out, '\n');
      kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
      KlInstruction extra = *pc++;
      print_prefix(code, out, pc - 1, get_instname(extra));
      print_XI(out, extra);
      ko_printf(out, "jump to %u if %s", pc + KLINST_XI_GETI(extra) - code->code, KLINST_XI_GETX(extra) ? "true" : "false");
      return pc;
    }
    case KLOPCODE_LEC: {
      print_AX(out, inst);
      print_constant(code, out, &code->constants[KLINST_AX_GETX(inst)]);
      ko_putc(out, '\n');
      kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
      KlInstruction extra = *pc++;
      print_prefix(code, out, pc - 1, get_instname(extra));
      print_XI(out, extra);
      ko_printf(out, "jump to %u if %s", pc + KLINST_XI_GETI(extra) - code->code, KLINST_XI_GETX(extra) ? "true" : "false");
      return pc;
    }
    case KLOPCODE_GEC: {
      print_AX(out, inst);
      print_constant(code, out, &code->constants[KLINST_AX_GETX(inst)]);
      ko_putc(out, '\n');
      kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
      KlInstruction extra = *pc++;
      print_prefix(code, out, pc - 1, get_instname(extra));
      print_XI(out, extra);
      ko_printf(out, "jump to %u if %s", pc + KLINST_XI_GETI(extra) - code->code, KLINST_XI_GETX(extra) ? "true" : "false");
      return pc;
    }
    case KLOPCODE_EQI: {
      print_AI(out, inst);
      ko_printf(out, "%d\n", KLINST_AI_GETI(inst));
      kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
      KlInstruction extra = *pc++;
      print_prefix(code, out, pc - 1, get_instname(extra));
      print_XI(out, extra);
      ko_printf(out, "jump to %u if %s", pc + KLINST_XI_GETI(extra) - code->code, KLINST_XI_GETX(extra) ? "true" : "false");
      return pc;
    }
    case KLOPCODE_NEI: {
      print_AI(out, inst);
      ko_printf(out, "%d\n", KLINST_AI_GETI(inst));
      kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
      KlInstruction extra = *pc++;
      print_prefix(code, out, pc - 1, get_instname(extra));
      print_XI(out, extra);
      ko_printf(out, "jump to %u if %s", pc + KLINST_XI_GETI(extra) - code->code, KLINST_XI_GETX(extra) ? "true" : "false");
      return pc;
    }
    case KLOPCODE_LTI: {
      print_AI(out, inst);
      ko_printf(out, "%d\n", KLINST_AI_GETI(inst));
      kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
      KlInstruction extra = *pc++;
      print_prefix(code, out, pc - 1, get_instname(extra));
      print_XI(out, extra);
      ko_printf(out, "jump to %u if %s", pc + KLINST_XI_GETI(extra) - code->code, KLINST_XI_GETX(extra) ? "true" : "false");
      return pc;
    }
    case KLOPCODE_GTI: {
      print_AI(out, inst);
      ko_printf(out, "%d\n", KLINST_AI_GETI(inst));
      kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
      KlInstruction extra = *pc++;
      print_prefix(code, out, pc - 1, get_instname(extra));
      print_XI(out, extra);
      ko_printf(out, "jump to %u if %s", pc + KLINST_XI_GETI(extra) - code->code, KLINST_XI_GETX(extra) ? "true" : "false");
      return pc;
    }
    case KLOPCODE_LEI: {
      print_AI(out, inst);
      ko_printf(out, "%d\n", KLINST_AI_GETI(inst));
      kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
      KlInstruction extra = *pc++;
      print_prefix(code, out, pc - 1, get_instname(extra));
      print_XI(out, extra);
      ko_printf(out, "jump to %u if %s", pc + KLINST_XI_GETI(extra) - code->code, KLINST_XI_GETX(extra) ? "true" : "false");
      return pc;
    }
    case KLOPCODE_GEI: {
      print_AI(out, inst);
      ko_printf(out, "%d\n", KLINST_AI_GETI(inst));
      kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
      KlInstruction extra = *pc++;
      print_prefix(code, out, pc - 1, get_instname(extra));
      print_XI(out, extra);
      ko_printf(out, "jump to %u if %s", pc + KLINST_XI_GETI(extra) - code->code, KLINST_XI_GETX(extra) ? "true" : "false");
      return pc;
    }
    case KLOPCODE_MATCH: {
      print_AX(out, inst);
      ko_printf(out, "must match ", KLINST_AI_GETI(inst));
      print_constant(code, out, &code->constants[KLINST_AX_GETX(inst)]);
      return pc;
    }
    case KLOPCODE_PMARR: {
      print_ABX(out, inst);
      ko_printf(out, "match R%u to R%u, front: %u\n", KLINST_ABX_GETB(inst), KLINST_ABX_GETA(inst), KLINST_ABX_GETX(inst));
      kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_EXTRA, "");
      KlInstruction extra = *pc++;
      print_prefix(code, out, pc - 1, "PMARR EXTRA");
      print_XI(out, extra);
      ko_printf(out, "back: %u; jump to %u if not an array", KLINST_XI_GETX(extra), pc + KLINST_XI_GETI(extra) - code->code);
      return pc;
    }
    case KLOPCODE_PBARR: {
      print_ABX(out, inst);
      ko_printf(out, "bind R%u to R%u, front: %u\n", KLINST_ABX_GETB(inst), KLINST_ABX_GETA(inst), KLINST_ABX_GETX(inst));
      kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_EXTRA, "");
      KlInstruction extra = *pc++;
      print_prefix(code, out, pc - 1, "PBARR EXTRA");
      print_XI(out, extra);
      ko_printf(out, "back: %u", KLINST_XI_GETX(extra));
      return pc;
    }
    case KLOPCODE_PMTUP: {
      print_ABX(out, inst);
      ko_printf(out, "match R%u to R%u, %u elements\n", KLINST_ABX_GETB(inst), KLINST_ABX_GETA(inst), KLINST_ABX_GETX(inst));
      kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_EXTRA, "");
      KlInstruction extra = *pc++;
      print_prefix(code, out, pc - 1, "PMTUP EXTRA");
      print_XI(out, extra);
      ko_printf(out, "jump to %u if not a tuple", pc + KLINST_XI_GETI(extra) - code->code);
      return pc;
    }
    case KLOPCODE_PBTUP: {
      print_ABX(out, inst);
      ko_printf(out, "bind R%u to R%u, %u values", KLINST_ABX_GETB(inst), KLINST_ABX_GETA(inst), KLINST_ABX_GETX(inst));
      return pc;
    }
    case KLOPCODE_PMMAP: {
      print_ABX(out, inst);
      ko_printf(out, "match R%u to R%u, %u elements\n", KLINST_ABX_GETB(inst), KLINST_ABX_GETA(inst), KLINST_ABX_GETX(inst));
      kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_PMAPPOST, "");
      KlInstruction extra = *pc++;
      print_prefix(code, out, pc - 1, get_instname(extra));
      print_XI(out, extra);
      ko_printf(out, "jump to %u if not map", pc + KLINST_XI_GETI(extra) - code->code);
      return pc;
    }
    case KLOPCODE_PBMAP: {
      print_ABX(out, inst);
      ko_printf(out, "bind R%u to R%u, %u elements\n", KLINST_ABX_GETB(inst), KLINST_ABX_GETA(inst), KLINST_ABX_GETX(inst));
      kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_PMAPPOST, "");
      KlInstruction extra = *pc++;
      print_prefix(code, out, pc - 1, get_instname(extra));
      print_XI(out, extra);
      return pc;
    }
    case KLOPCODE_PMAPPOST: {
      print_XI(out, inst);
      ko_printf(out, "this instruction should not be printed in this way");
      return pc;
    }
    case KLOPCODE_PMOBJ:
    case KLOPCODE_PBOBJ: {
      print_ABX(out, inst);
      ko_printf(out, "bind R%u to R%u, %u fields", KLINST_ABX_GETB(inst), KLINST_ABX_GETA(inst), KLINST_ABX_GETX(inst));
      return pc;
    }
    case KLOPCODE_NEWOBJ: {
      print_AB(out, inst);
      return pc;
    }
    case KLOPCODE_ADJUSTARGS: {
      print_(out, inst);
      return pc;
    }
    case KLOPCODE_VFORPREP: {
      print_AI(out, inst);
      ko_printf(out, "jump to %u if finished", pc + KLINST_AI_GETI(inst) - code->code);
      return pc;
    }
    case KLOPCODE_VFORLOOP: {
      print_AI(out, inst);
      ko_printf(out, "jump to %u if continue", pc + KLINST_AI_GETI(inst) - code->code);
      return pc;
    }
    case KLOPCODE_IFORPREP: {
      print_AI(out, inst);
      ko_printf(out, "jump to %u if finished", pc + KLINST_AI_GETI(inst) - code->code);
      return pc;
    }
    case KLOPCODE_IFORLOOP: {
      print_AI(out, inst);
      ko_printf(out, "jump to %u if continue", pc + KLINST_AI_GETI(inst) - code->code);
      return pc;
    }
    case KLOPCODE_GFORPREP: {
      print_AX(out, inst);
      ko_printf(out, "iterable object at R%u, %u iteration variables\n", KLINST_AX_GETA(inst), KLINST_AX_GETX(inst));
      KlInstruction extra = *pc++;

      print_prefix(code, out, pc - 1, "FALSEJMP");
      print_AI(out, extra);
      ko_printf(out, "jump to %u if iteration finished", pc + KLINST_AI_GETI(extra) - code->code);
      return pc;
    }
    case KLOPCODE_GFORLOOP: {
      print_AX(out, inst);
      ko_printf(out, "%u results", KLINST_AX_GETX(inst));
      /* following TRUEJMP just printed by next call of this function */
      return pc;
    }
    case KLOPCODE_ASYNC: {
      print_AB(out, inst);
      return pc;
    }
    case KLOPCODE_YIELD: {
      print_AXY(out, inst);
      size_t nres = KLINST_AXY_GETX(inst);
      size_t nwanted = KLINST_AXY_GETY(inst);
      nres == KLINST_VARRES ? ko_printf(out, "all results yielded, ") : ko_printf(out, "%u results yielded, ", nres);
      nwanted == KLINST_VARRES ? ko_printf(out, "all results wanted") : ko_printf(out, "%u results wanted", nwanted);
      return pc;
    }
    case KLOPCODE_VARARG: {
      print_AXY(out, inst);
      size_t nwanted = KLINST_AXY_GETX(inst);
      nwanted == KLINST_VARRES ? ko_printf(out, "all arguments to R%u", KLINST_AXY_GETA(inst))
                               : ko_printf(out, "%u arguments to R%u", nwanted, KLINST_AXY_GETA(inst));
      return pc;
    }
    default: {
      kl_assert(false, "control flow should not reach here");
      return pc;
    }
  }
}

static void print_reftbl(const KlCode* code, Ko* out) {
  ko_printf(out, "%u references:\n", code->nref);
  for (size_t i = 0; i < code->nref; ++i)
    ko_printf(out, "reference %4zu: from %10s, index = %u\n", i, code->refinfo[i].on_stack ? "stack" : "ref table", (unsigned)code->refinfo[i].index);
}

typedef struct tagKlCodePrintInfo {
  size_t idx;
  struct tagKlCodePrintInfo* upper;
} KlCodePrintInfo;

static void print_printinfo(const KlCodePrintInfo* printinfo, Ko* out) {
  if (printinfo) {
    print_printinfo(printinfo->upper, out);
    ko_printf(out, ":%zu", printinfo->idx);
  }
}

void klcode_print_function(const KlCode* code, Ko* out, KlCodePrintInfo* printinfo) {
  KlInstruction* pc = code->code;
  KlInstruction* end = code->code + code->codelen;
  ko_printf(out, "function(top");
  print_printinfo(printinfo, out);
  ko_printf(out, "), %u registers\n", code->framesize);
  ko_printf(out, "%u instructions:\n", code->codelen);
  while (pc != end) {
    pc = print_instruction(code, out, pc);
    ko_putc(out, '\n');
  }
  ko_putc(out, '\n');
  ko_printf(out, "%u constants:\n", code->nconst);
  for (size_t i = 0; i < code->nconst; ++i) {
    ko_printf(out, "constant %4zu: ", i);
    print_constant(code, out, &code->constants[i]);
    ko_putc(out, '\n');
  }
  ko_putc(out, '\n');
  print_reftbl(code, out);
  ko_putc(out, '\n');
  if (code->nnested == 0) {
    ko_printf(out, "0 sub-function\n");
    return;
  }
  ko_printf(out, "%u sub-functions:\n", code->nnested);
  for (size_t i = 0; i < code->nnested; ++i) {
    ko_putc(out, '\n');
    KlCodePrintInfo subfunc = { .idx = i, .upper = printinfo };
    klcode_print_function(code->nestedfunc[i], out, &subfunc);
  }
}

void klcode_print(const KlCode* code, Ko* out) {
  ko_printf(out, "source file: %.*s\n", code->srcfile.length, klstrtbl_getstring(code->strtbl, code->srcfile.id));
  klcode_print_function(code, out, NULL);
}
