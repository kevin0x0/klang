#include "include/code/klcode.h"
#include "include/misc/klutils.h"

static void klcode_print_prefix(KlCode* code, Ko* out, KlInstruction* pc, const char* name) {
  code->posinfo ? ko_printf(out, "(%4u, %4u) [%03u] %-15s", code->posinfo[pc - code->code].begin, code->posinfo[pc - code->code].end, pc - code->code, name)
                 : ko_printf(out, "(no position info) [%03u] %-15s", pc - code->code, name);
}

static const char* klcode_get_instname(KlInstruction inst) {
  static const char* names[KLOPCODE_NINST] = {
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
    [KLOPCODE_MKMAP] = "MKMAP",
    [KLOPCODE_MKARRAY] = "MKARRAY",
    [KLOPCODE_MKCLOSURE] = "MKCLOSURE",
    [KLOPCODE_MKMETHOD] = "MKMETHOD",
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
    [KLOPCODE_GFORLOOP] = "GFORLOOP",
    [KLOPCODE_ASYNC] = "ASYNC",
    [KLOPCODE_YIELD] = "YIELD",
    [KLOPCODE_VARARG] = "VARARG",
  };
  return names[KLINST_GET_OPCODE(inst)];
}

static void klcode_print_(Ko* out, KlInstruction inst) {
  kl_unused(inst);
  ko_printf(out, "| %4s %4s %4s %4s | ", "", "", "", "");
}

static void klcode_print_A(Ko* out, KlInstruction inst) {
  ko_printf(out, "| %4u %4s %4s %4s | ", (unsigned)KLINST_A_GETA(inst), "", "", "");
}

static void klcode_print_AB(Ko* out, KlInstruction inst) {
  ko_printf(out, "| %4u %4u %4s %4s | ", (unsigned)KLINST_ABC_GETA(inst), (unsigned)KLINST_ABC_GETB(inst), "", "");
}

static void klcode_print_AX(Ko* out, KlInstruction inst) {
  ko_printf(out, "| %4u %4u %4s %4s | ", (unsigned)KLINST_AX_GETA(inst), (unsigned)KLINST_AX_GETX(inst), "", "");
}

static void klcode_print_ABC(Ko* out, KlInstruction inst) {
  ko_printf(out, "| %4u %4u %4u %4s | ", (unsigned)KLINST_ABC_GETA(inst), (unsigned)KLINST_ABC_GETB(inst), (unsigned)KLINST_ABC_GETC(inst), "");
}

static void klcode_print_ABX(Ko* out, KlInstruction inst) {
  ko_printf(out, "| %4u %4u %4u %4s | ", (unsigned)KLINST_ABX_GETA(inst), (unsigned)KLINST_ABX_GETB(inst), (unsigned)KLINST_ABX_GETX(inst), "");
}

static void klcode_print_ABTX(Ko* out, KlInstruction inst) {
  ko_printf(out, "| %4u %4u %4u %4u | ", (unsigned)KLINST_ABTX_GETA(inst), (unsigned)KLINST_ABTX_GETB(inst), (unsigned)(KLINST_ABTX_GETT(inst) != 0), (unsigned)KLINST_ABTX_GETX(inst));
}

static void klcode_print_AXY(Ko* out, KlInstruction inst) {
  ko_printf(out, "| %4u %4u %4u %4s | ", (unsigned)KLINST_AXY_GETA(inst), (unsigned)KLINST_AXY_GETX(inst), (unsigned)KLINST_AXY_GETY(inst), "");
}

static void klcode_print_XYZ(Ko* out, KlInstruction inst) {
  ko_printf(out, "| %4u %4u %4u %4s | ", (unsigned)KLINST_XYZ_GETX(inst), (unsigned)KLINST_XYZ_GETY(inst), (unsigned)KLINST_XYZ_GETZ(inst), "");
}

static void klcode_print_ABI(Ko* out, KlInstruction inst) {
  ko_printf(out, "| %4u %4u %4d %4s | ", (unsigned)KLINST_ABI_GETA(inst), (unsigned)KLINST_ABI_GETB(inst), (signed)KLINST_ABI_GETI(inst), "");
}

static void klcode_print_AI(Ko* out, KlInstruction inst) {
  ko_printf(out, "| %4u %4d %4s %4s | ", (unsigned)KLINST_AI_GETA(inst), (signed)KLINST_AI_GETI(inst), "", "");
}

static void klcode_print_XI(Ko* out, KlInstruction inst) {
  ko_printf(out, "| %4u %4d %4s %4s | ", (unsigned)KLINST_XI_GETX(inst), (signed)KLINST_AI_GETI(inst), "", "");
}

static void klcode_print_I(Ko* out, KlInstruction inst) {
  ko_printf(out, "| %4d %4s %4s %4s | ", (signed)KLINST_I_GETI(inst), "", "", "");
}

static void klcode_print_constant(KlCode* code, Ko* out, KlConstant* constant) {
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
    default: {
      kl_assert(false, "impossible constant type");
    }
  }
}

static void klcode_print_string_noquote(KlCode* code, Ko* out, KlConstant* constant) {
  kl_assert(constant->type == KLC_STRING, "");
  ko_printf(out, "%.*s", constant->string.length, klstrtbl_getstring(code->strtbl, constant->string.id));
}

static KlInstruction* klcode_print_instruction(KlCode* code, Ko* out, KlInstruction* pc) {
  KlInstruction inst = *pc++;
  klcode_print_prefix(code, out, pc - 1, klcode_get_instname(inst));
  uint8_t opcode = KLINST_GET_OPCODE(inst);
  switch (opcode) {
    case KLOPCODE_MOVE: {
      klcode_print_AB(out, inst);
      return pc;
    }
    case KLOPCODE_MULTIMOVE: {
      klcode_print_ABX(out, inst);
      size_t nmove = KLINST_ABX_GETX(inst);
      nmove == KLINST_VARRES ? ko_printf(out, "move all") : ko_printf(out, "move %u values", nmove);
      return pc;
    }
    case KLOPCODE_ADD: {
      klcode_print_ABC(out, inst);
      return pc;
    }
    case KLOPCODE_SUB: {
      klcode_print_ABC(out, inst);
      return pc;
    }
    case KLOPCODE_MUL: {
      klcode_print_ABC(out, inst);
      return pc;
    }
    case KLOPCODE_DIV: {
      klcode_print_ABC(out, inst);
      return pc;
    }
    case KLOPCODE_MOD: {
      klcode_print_ABC(out, inst);
      return pc;
    }
    case KLOPCODE_IDIV: {
      klcode_print_ABC(out, inst);
      return pc;
    }
    case KLOPCODE_CONCAT: {
      klcode_print_ABC(out, inst);
      return pc;
    }
    case KLOPCODE_ADDI: {
      klcode_print_ABI(out, inst);
      ko_printf(out, "%d", KLINST_ABI_GETI(inst));
      return pc;
    }
    case KLOPCODE_SUBI: {
      klcode_print_ABI(out, inst);
      ko_printf(out, "%d", KLINST_ABI_GETI(inst));
      return pc;
    }
    case KLOPCODE_MULI: {
      klcode_print_ABI(out, inst);
      ko_printf(out, "%d", KLINST_ABI_GETI(inst));
      return pc;
    }
    case KLOPCODE_DIVI: {
      klcode_print_ABI(out, inst);
      ko_printf(out, "%d", KLINST_ABI_GETI(inst));
      return pc;
    }
    case KLOPCODE_MODI: {
      klcode_print_ABI(out, inst);
      ko_printf(out, "%d", KLINST_ABI_GETI(inst));
      return pc;
    }
    case KLOPCODE_IDIVI: {
      klcode_print_ABI(out, inst);
      ko_printf(out, "%d", KLINST_ABI_GETI(inst));
      return pc;
    }
    case KLOPCODE_ADDC: {
      klcode_print_ABX(out, inst);
      klcode_print_constant(code, out, &code->constants[KLINST_ABX_GETX(inst)]);
      return pc;
    }
    case KLOPCODE_SUBC: {
      klcode_print_ABX(out, inst);
      klcode_print_constant(code, out, &code->constants[KLINST_ABX_GETX(inst)]);
      return pc;
    }
    case KLOPCODE_MULC: {
      klcode_print_ABX(out, inst);
      klcode_print_constant(code, out, &code->constants[KLINST_ABX_GETX(inst)]);
      return pc;
    }
    case KLOPCODE_DIVC: {
      klcode_print_ABX(out, inst);
      klcode_print_constant(code, out, &code->constants[KLINST_ABX_GETX(inst)]);
      return pc;
    }
    case KLOPCODE_MODC: {
      klcode_print_ABX(out, inst);
      klcode_print_constant(code, out, &code->constants[KLINST_ABX_GETX(inst)]);
      return pc;
    }
    case KLOPCODE_IDIVC: {
      klcode_print_ABX(out, inst);
      klcode_print_constant(code, out, &code->constants[KLINST_ABX_GETX(inst)]);
      return pc;
    }
    case KLOPCODE_NEG: {
      klcode_print_A(out, inst);
      return pc;
    }
    case KLOPCODE_SCALL: {
      klcode_print_AXY(out, inst);
      size_t narg = KLINST_AXY_GETX(inst);
      size_t nret = KLINST_AXY_GETY(inst);
      narg == KLINST_VARRES ? ko_printf(out, "all arguments, ") : ko_printf(out, "%u arguments, ", narg);
      nret == KLINST_VARRES ? ko_printf(out, "all results") : ko_printf(out, "%u results", nret);
      return pc;
    }
    case KLOPCODE_CALL: {
      klcode_print_A(out, inst);
      ko_printf(out, "callable object at R%u\n", KLINST_AXY_GETA(inst));
      KlInstruction extra = *pc++;

      klcode_print_prefix(code, out, pc - 1, "CALL EXTRA");
      klcode_print_XYZ(out, extra);
      size_t narg = KLINST_XYZ_GETX(extra);
      size_t nret = KLINST_XYZ_GETY(extra);
      size_t target = KLINST_XYZ_GETZ(extra);
      narg == KLINST_VARRES ? ko_printf(out, "all arguments, ") : ko_printf(out, "%u arguments, ", narg);
      nret == KLINST_VARRES ? ko_printf(out, "all results, ") : ko_printf(out, "%u results, ", nret);
      ko_printf(out, "target at R%u", target);
      return pc;
    }
    case KLOPCODE_METHOD: {
      klcode_print_AX(out, inst);
      ko_printf(out, "object at R%u, method name: ", KLINST_AX_GETA(inst));
      klcode_print_string_noquote(code, out, &code->constants[KLINST_AX_GETX(inst)]);
      ko_putc(out, '\n');
      KlInstruction extra = *pc++;

      klcode_print_prefix(code, out, pc - 1, "METHOD EXTRA");
      klcode_print_XYZ(out, extra);
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
      klcode_print_AX(out, inst);
      nres == KLINST_VARRES ? ko_printf(out, "all results") : ko_printf(out, "%u results", nres);
      return pc;
    }
    case KLOPCODE_RETURN0: {
      klcode_print_(out, inst);
      return pc;
    }
    case KLOPCODE_RETURN1: {
      klcode_print_A(out, inst);
      return pc;
    }
    case KLOPCODE_LOADBOOL: {
      klcode_print_AX(out, inst);
      ko_printf(out, KLINST_AX_GETX(inst) ? "true" : "false");
      return pc;
    }
    case KLOPCODE_LOADI: {
      klcode_print_AI(out, inst);
      ko_printf(out, "%d", KLINST_AI_GETI(inst));
      return pc;
    }
    case KLOPCODE_LOADC: {
      klcode_print_AX(out, inst);
      klcode_print_constant(code, out, &code->constants[KLINST_AX_GETX(inst)]);
      return pc;
    }
    case KLOPCODE_LOADNIL: {
      klcode_print_AX(out, inst);
      ko_printf(out, "%u nils", KLINST_AX_GETX(inst));
      return pc;
    }
    case KLOPCODE_LOADREF: {
      klcode_print_AX(out, inst);
      return pc;
    }
    case KLOPCODE_LOADGLOBAL: {
      klcode_print_AX(out, inst);
      klcode_print_string_noquote(code, out, &code->constants[KLINST_AX_GETX(inst)]);
      return pc;
    }
    case KLOPCODE_STOREREF: {
      klcode_print_AX(out, inst);
      return pc;
    }
    case KLOPCODE_STOREGLOBAL: {
      klcode_print_AX(out, inst);
      klcode_print_string_noquote(code, out, &code->constants[KLINST_AX_GETX(inst)]);
      return pc;
    }
    case KLOPCODE_MKMAP: {
      /* this instruction tells us current stack top */
      size_t stktop = KLINST_ABX_GETB(inst);
      size_t capacity = KLINST_ABX_GETX(inst);
      klcode_print_ABC(out, inst);
      ko_printf(out, "capacity: %u, stack top: %u", klbit(capacity), stktop);
      return pc;
    }
    case KLOPCODE_MKARRAY: {
      klcode_print_ABX(out, inst);
      size_t nelem = KLINST_ABX_GETX(inst);
      nelem == KLINST_VARRES ? ko_printf(out, "many elements") : ko_printf(out, "%u elements", nelem);
      return pc;
    }
    case KLOPCODE_MKMETHOD:
    case KLOPCODE_MKCLOSURE: {
      klcode_print_AX(out, inst);
      return pc;
    }
    case KLOPCODE_APPEND: {
      klcode_print_ABX(out, inst);
      size_t nelem = KLINST_ABX_GETX(inst);
      nelem == KLINST_VARRES ? ko_printf(out, "many elements") : ko_printf(out, "%u elements", nelem);
      return pc;
    }
    case KLOPCODE_MKCLASS: {
      /* this instruction tells us current stack top */
      size_t stktop = KLINST_ABTX_GETB(inst);
      size_t capacity = KLINST_ABTX_GETX(inst);
      klcode_print_ABTX(out, inst);
      ko_printf(out, "capacity: %u, stack top: %u", klbit(capacity), stktop);
      if (KLINST_ABTX_GETT(inst))
        ko_printf(out, ", parent at R%u", stktop);
      return pc;
    }
    case KLOPCODE_INDEXI: {
      klcode_print_ABI(out, inst);
      ko_printf(out, "R%u = R%u[%d]", KLINST_ABI_GETA(inst), KLINST_ABI_GETB(inst), KLINST_ABI_GETI(inst));
      return pc;
    }
    case KLOPCODE_INDEXASI: {
      klcode_print_ABI(out, inst);
      ko_printf(out, "R%u[%d] = R%u", KLINST_ABI_GETB(inst), KLINST_ABI_GETI(inst), KLINST_ABI_GETA(inst));
      return pc;
    }
    case KLOPCODE_INDEX: {
      klcode_print_ABC(out, inst);
      ko_printf(out, "R%u = R%u[R%d]", KLINST_ABC_GETA(inst), KLINST_ABC_GETB(inst), KLINST_ABC_GETC(inst));
      return pc;
    }
    case KLOPCODE_INDEXAS: {
      klcode_print_ABC(out, inst);
      ko_printf(out, "R%u[%d] = R%u", KLINST_ABC_GETB(inst), KLINST_ABC_GETC(inst), KLINST_ABC_GETA(inst));
      return pc;
    }
    case KLOPCODE_GETFIELDR: {
      klcode_print_ABC(out, inst);
      ko_printf(out, "R%u = R%u.R%d", KLINST_ABC_GETA(inst), KLINST_ABC_GETB(inst), KLINST_ABC_GETC(inst));
      return pc;
    }
    case KLOPCODE_GETFIELDC: {
      klcode_print_ABC(out, inst);
      ko_printf(out, "R%u = R%u.", KLINST_ABC_GETA(inst), KLINST_ABC_GETB(inst));
      klcode_print_string_noquote(code, out, &code->constants[KLINST_ABC_GETC(inst)]);
      return pc;
    }
    case KLOPCODE_SETFIELDR: {
      klcode_print_ABC(out, inst);
      ko_printf(out, "R%u.R%u = R%u", KLINST_ABC_GETB(inst), KLINST_ABC_GETC(inst), KLINST_ABC_GETA(inst));
      return pc;
    }
    case KLOPCODE_SETFIELDC: {
      klcode_print_ABC(out, inst);
      ko_printf(out, "R%u.", KLINST_ABC_GETB(inst));
      klcode_print_string_noquote(code, out, &code->constants[KLINST_ABC_GETC(inst)]);
      ko_printf(out, " = R%u", KLINST_ABC_GETA(inst));
      return pc;
    }
    case KLOPCODE_REFGETFIELDR: {
      klcode_print_ABC(out, inst);
      ko_printf(out, "R%u = REF%u.R%u", KLINST_ABX_GETA(inst), KLINST_ABX_GETX(inst), KLINST_ABX_GETB(inst));
      return pc;
    }
    case KLOPCODE_REFGETFIELDC: {
      klcode_print_ABC(out, inst);
      ko_printf(out, "R%u = REF%u.", KLINST_AXY_GETA(inst), KLINST_AXY_GETY(inst)); 
      klcode_print_string_noquote(code, out, &code->constants[KLINST_AXY_GETX(inst)]);
      return pc;
    }
    case KLOPCODE_REFSETFIELDR: {
      klcode_print_ABC(out, inst);
      ko_printf(out, "REF%u.R%u = R%u", KLINST_AXY_GETX(inst), KLINST_AXY_GETY(inst), KLINST_AXY_GETA(inst));
      return pc;
    }
    case KLOPCODE_REFSETFIELDC: {
      klcode_print_ABC(out, inst);
      ko_printf(out, "REF%u.", KLINST_AXY_GETX(inst));
      klcode_print_string_noquote(code, out, &code->constants[KLINST_AXY_GETY(inst)]);
      ko_printf(out, " = R%u", KLINST_AXY_GETA(inst));
      return pc;
    }
    case KLOPCODE_NEWLOCAL: {
      klcode_print_AX(out, inst);
      klcode_print_constant(code, out, &code->constants[KLINST_AX_GETX(inst)]);
      return pc;
    }
    case KLOPCODE_LOADFALSESKIP: {
      klcode_print_A(out, inst);
      return pc;
    }
    case KLOPCODE_TESTSET: {
      klcode_print_AB(out, inst);
      ko_putc(out, '\n');
      kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
      KlInstruction extra = *pc++;
      klcode_print_prefix(code, out, pc - 1, klcode_get_instname(extra));
      klcode_print_XI(out, extra);
      ko_printf(out, "set R%u = R%u and jump to %u if %s", KLINST_ABC_GETA(inst), KLINST_ABC_GETB(inst), pc + KLINST_XI_GETI(extra) - code->code, KLINST_XI_GETX(extra) ? "true" : "false");
      return pc;
    }
    case KLOPCODE_TRUEJMP: {
      klcode_print_AI(out, inst);
      ko_printf(out, "jump to %d if true", pc + KLINST_AI_GETI(inst) - code->code);
      return pc;
    }
    case KLOPCODE_FALSEJMP: {
      klcode_print_AI(out, inst);
      ko_printf(out, "jump to %u if false", pc + KLINST_AI_GETI(inst) - code->code);
      return pc;
    }
    case KLOPCODE_JMP: {
      klcode_print_I(out, inst);
      ko_printf(out, "jump to %u", pc + KLINST_I_GETI(inst) - code->code);
      return pc;
    }
    case KLOPCODE_CONDJMP: {
      klcode_print_XI(out, inst);
      ko_printf(out, "this instruction should not be printed");
      return pc;
    }
    case KLOPCODE_CLOSEJMP: {
      klcode_print_XI(out, inst);
      ko_printf(out, "close from %u then jump to %u", KLINST_XI_GETX(inst), pc + KLINST_XI_GETI(inst) - code->code);
      return pc;
      break;
    }
    case KLOPCODE_HASFIELD: {
      klcode_print_AX(out, inst);
      klcode_print_constant(code, out, &code->constants[KLINST_AX_GETX(inst)]);
      ko_putc(out, '\n');
      kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
      KlInstruction extra = *pc++;
      klcode_print_prefix(code, out, pc - 1, klcode_get_instname(extra));
      klcode_print_XI(out, extra);
      ko_printf(out, "jump to %u if %s", pc + KLINST_XI_GETI(extra) - code->code, KLINST_XI_GETX(extra) ? "true" : "false");
      return pc;
    }
    case KLOPCODE_IS: {
      klcode_print_AB(out, inst);
      ko_putc(out, '\n');
      kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
      KlInstruction extra = *pc++;
      klcode_print_prefix(code, out, pc - 1, klcode_get_instname(extra));
      klcode_print_XI(out, extra);
      ko_printf(out, "jump to %u if %s", pc + KLINST_XI_GETI(extra) - code->code, KLINST_XI_GETX(extra) ? "true" : "false");
      return pc;
    }
    case KLOPCODE_EQ: {
      klcode_print_AB(out, inst);
      ko_putc(out, '\n');
      kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
      KlInstruction extra = *pc++;
      klcode_print_prefix(code, out, pc - 1, klcode_get_instname(extra));
      klcode_print_XI(out, extra);
      ko_printf(out, "jump to %u if %s", pc + KLINST_XI_GETI(extra) - code->code, KLINST_XI_GETX(extra) ? "true" : "false");
      return pc;
    }
    case KLOPCODE_NE: {
      klcode_print_AB(out, inst);
      ko_putc(out, '\n');
      kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
      KlInstruction extra = *pc++;
      klcode_print_prefix(code, out, pc - 1, klcode_get_instname(extra));
      klcode_print_XI(out, extra);
      ko_printf(out, "jump to %u if %s", pc + KLINST_XI_GETI(extra) - code->code, KLINST_XI_GETX(extra) ? "true" : "false");
      return pc;
    }
    case KLOPCODE_LT: {
      klcode_print_AB(out, inst);
      ko_putc(out, '\n');
      kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
      KlInstruction extra = *pc++;
      klcode_print_prefix(code, out, pc - 1, klcode_get_instname(extra));
      klcode_print_XI(out, extra);
      ko_printf(out, "jump to %u if %s", pc + KLINST_XI_GETI(extra) - code->code, KLINST_XI_GETX(extra) ? "true" : "false");
      return pc;
    }
    case KLOPCODE_GT: {
      klcode_print_AB(out, inst);
      ko_putc(out, '\n');
      kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
      KlInstruction extra = *pc++;
      klcode_print_prefix(code, out, pc - 1, klcode_get_instname(extra));
      klcode_print_XI(out, extra);
      ko_printf(out, "jump to %u if %s", pc + KLINST_XI_GETI(extra) - code->code, KLINST_XI_GETX(extra) ? "true" : "false");
      return pc;
    }
    case KLOPCODE_LE: {
      klcode_print_AB(out, inst);
      ko_putc(out, '\n');
      kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
      KlInstruction extra = *pc++;
      klcode_print_prefix(code, out, pc - 1, klcode_get_instname(extra));
      klcode_print_XI(out, extra);
      ko_printf(out, "jump to %u if %s", pc + KLINST_XI_GETI(extra) - code->code, KLINST_XI_GETX(extra) ? "true" : "false");
      return pc;
    }
    case KLOPCODE_GE: {
      klcode_print_AB(out, inst);
      ko_putc(out, '\n');
      kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
      KlInstruction extra = *pc++;
      klcode_print_prefix(code, out, pc - 1, klcode_get_instname(extra));
      klcode_print_XI(out, extra);
      ko_printf(out, "jump to %u if %s", pc + KLINST_XI_GETI(extra) - code->code, KLINST_XI_GETX(extra) ? "true" : "false");
      return pc;
    }
    case KLOPCODE_EQC: {
      klcode_print_AX(out, inst);
      klcode_print_constant(code, out, &code->constants[KLINST_AX_GETX(inst)]);
      ko_putc(out, '\n');
      kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
      KlInstruction extra = *pc++;
      klcode_print_prefix(code, out, pc - 1, klcode_get_instname(extra));
      klcode_print_XI(out, extra);
      ko_printf(out, "jump to %u if %s", pc + KLINST_XI_GETI(extra) - code->code, KLINST_XI_GETX(extra) ? "true" : "false");
      return pc;
    }
    case KLOPCODE_NEC: {
      klcode_print_AX(out, inst);
      klcode_print_constant(code, out, &code->constants[KLINST_AX_GETX(inst)]);
      ko_putc(out, '\n');
      kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
      KlInstruction extra = *pc++;
      klcode_print_prefix(code, out, pc - 1, klcode_get_instname(extra));
      klcode_print_XI(out, extra);
      ko_printf(out, "jump to %u if %s", pc + KLINST_XI_GETI(extra) - code->code, KLINST_XI_GETX(extra) ? "true" : "false");
      return pc;
    }
    case KLOPCODE_LTC: {
      klcode_print_AX(out, inst);
      klcode_print_constant(code, out, &code->constants[KLINST_AX_GETX(inst)]);
      ko_putc(out, '\n');
      kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
      KlInstruction extra = *pc++;
      klcode_print_prefix(code, out, pc - 1, klcode_get_instname(extra));
      klcode_print_XI(out, extra);
      ko_printf(out, "jump to %u if %s", pc + KLINST_XI_GETI(extra) - code->code, KLINST_XI_GETX(extra) ? "true" : "false");
      return pc;
    }
    case KLOPCODE_GTC: {
      klcode_print_AX(out, inst);
      klcode_print_constant(code, out, &code->constants[KLINST_AX_GETX(inst)]);
      ko_putc(out, '\n');
      kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
      KlInstruction extra = *pc++;
      klcode_print_prefix(code, out, pc - 1, klcode_get_instname(extra));
      klcode_print_XI(out, extra);
      ko_printf(out, "jump to %u if %s", pc + KLINST_XI_GETI(extra) - code->code, KLINST_XI_GETX(extra) ? "true" : "false");
      return pc;
    }
    case KLOPCODE_LEC: {
      klcode_print_AX(out, inst);
      klcode_print_constant(code, out, &code->constants[KLINST_AX_GETX(inst)]);
      ko_putc(out, '\n');
      kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
      KlInstruction extra = *pc++;
      klcode_print_prefix(code, out, pc - 1, klcode_get_instname(extra));
      klcode_print_XI(out, extra);
      ko_printf(out, "jump to %u if %s", pc + KLINST_XI_GETI(extra) - code->code, KLINST_XI_GETX(extra) ? "true" : "false");
      return pc;
    }
    case KLOPCODE_GEC: {
      klcode_print_AX(out, inst);
      klcode_print_constant(code, out, &code->constants[KLINST_AX_GETX(inst)]);
      ko_putc(out, '\n');
      kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
      KlInstruction extra = *pc++;
      klcode_print_prefix(code, out, pc - 1, klcode_get_instname(extra));
      klcode_print_XI(out, extra);
      ko_printf(out, "jump to %u if %s", pc + KLINST_XI_GETI(extra) - code->code, KLINST_XI_GETX(extra) ? "true" : "false");
      return pc;
    }
    case KLOPCODE_EQI: {
      klcode_print_AI(out, inst);
      ko_printf(out, "%d\n", KLINST_AI_GETI(inst));
      kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
      KlInstruction extra = *pc++;
      klcode_print_prefix(code, out, pc - 1, klcode_get_instname(extra));
      klcode_print_XI(out, extra);
      ko_printf(out, "jump to %u if %s", pc + KLINST_XI_GETI(extra) - code->code, KLINST_XI_GETX(extra) ? "true" : "false");
      return pc;
    }
    case KLOPCODE_NEI: {
      klcode_print_AI(out, inst);
      ko_printf(out, "%d\n", KLINST_AI_GETI(inst));
      kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
      KlInstruction extra = *pc++;
      klcode_print_prefix(code, out, pc - 1, klcode_get_instname(extra));
      klcode_print_XI(out, extra);
      ko_printf(out, "jump to %u if %s", pc + KLINST_XI_GETI(extra) - code->code, KLINST_XI_GETX(extra) ? "true" : "false");
      return pc;
    }
    case KLOPCODE_LTI: {
      klcode_print_AI(out, inst);
      ko_printf(out, "%d\n", KLINST_AI_GETI(inst));
      kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
      KlInstruction extra = *pc++;
      klcode_print_prefix(code, out, pc - 1, klcode_get_instname(extra));
      klcode_print_XI(out, extra);
      ko_printf(out, "jump to %u if %s", pc + KLINST_XI_GETI(extra) - code->code, KLINST_XI_GETX(extra) ? "true" : "false");
      return pc;
    }
    case KLOPCODE_GTI: {
      klcode_print_AI(out, inst);
      ko_printf(out, "%d\n", KLINST_AI_GETI(inst));
      kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
      KlInstruction extra = *pc++;
      klcode_print_prefix(code, out, pc - 1, klcode_get_instname(extra));
      klcode_print_XI(out, extra);
      ko_printf(out, "jump to %u if %s", pc + KLINST_XI_GETI(extra) - code->code, KLINST_XI_GETX(extra) ? "true" : "false");
      return pc;
    }
    case KLOPCODE_LEI: {
      klcode_print_AI(out, inst);
      ko_printf(out, "%d\n", KLINST_AI_GETI(inst));
      kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
      KlInstruction extra = *pc++;
      klcode_print_prefix(code, out, pc - 1, klcode_get_instname(extra));
      klcode_print_XI(out, extra);
      ko_printf(out, "jump to %u if %s", pc + KLINST_XI_GETI(extra) - code->code, KLINST_XI_GETX(extra) ? "true" : "false");
      return pc;
    }
    case KLOPCODE_GEI: {
      klcode_print_AI(out, inst);
      ko_printf(out, "%d\n", KLINST_AI_GETI(inst));
      kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
      KlInstruction extra = *pc++;
      klcode_print_prefix(code, out, pc - 1, klcode_get_instname(extra));
      klcode_print_XI(out, extra);
      ko_printf(out, "jump to %u if %s", pc + KLINST_XI_GETI(extra) - code->code, KLINST_XI_GETX(extra) ? "true" : "false");
      return pc;
    }
    case KLOPCODE_MATCH: {
      klcode_print_AX(out, inst);
      ko_printf(out, "must match ", KLINST_AI_GETI(inst));
      klcode_print_constant(code, out, &code->constants[KLINST_AX_GETX(inst)]);
      return pc;
    }
    case KLOPCODE_PMARR: {
      klcode_print_ABX(out, inst);
      ko_printf(out, "match R%u to R%u, front: %u\n", KLINST_ABX_GETB(inst), KLINST_ABX_GETA(inst), KLINST_ABX_GETX(inst));
      kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_EXTRA, "");
      KlInstruction extra = *pc++;
      klcode_print_prefix(code, out, pc - 1, "PMARR EXTRA");
      klcode_print_XI(out, extra);
      ko_printf(out, "back: %u; jump to %u if not array", KLINST_XI_GETX(extra), pc + KLINST_XI_GETI(extra) - code->code);
      return pc;
    }
    case KLOPCODE_PBARR: {
      klcode_print_ABX(out, inst);
      ko_printf(out, "bind R%u to R%u, front: %u\n", KLINST_ABX_GETB(inst), KLINST_ABX_GETA(inst), KLINST_ABX_GETX(inst));
      kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_EXTRA, "");
      KlInstruction extra = *pc++;
      klcode_print_prefix(code, out, pc - 1, "PBARR EXTRA");
      klcode_print_XI(out, extra);
      ko_printf(out, "back: %u", KLINST_XI_GETX(extra));
      return pc;
    }
    case KLOPCODE_PMTUP: {
      klcode_print_ABX(out, inst);
      ko_printf(out, "match R%u to R%u, %u elements", KLINST_ABX_GETB(inst), KLINST_ABX_GETA(inst), KLINST_ABX_GETX(inst));
      kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_EXTRA, "");
      KlInstruction extra = *pc++;
      klcode_print_prefix(code, out, pc - 1, "PMTUP EXTRA");
      klcode_print_XI(out, extra);
      ko_printf(out, "jump to %u if not array", pc + KLINST_XI_GETI(extra) - code->code);
      return pc;
    }
    case KLOPCODE_PBTUP: {
      klcode_print_ABX(out, inst);
      ko_printf(out, "bind R%u to R%u, %u elements", KLINST_ABX_GETB(inst), KLINST_ABX_GETA(inst), KLINST_ABX_GETX(inst));
      return pc;
    }
    case KLOPCODE_PMMAP: {
      klcode_print_ABX(out, inst);
      ko_printf(out, "match R%u to R%u, %u elements\n", KLINST_ABX_GETB(inst), KLINST_ABX_GETA(inst), KLINST_ABX_GETX(inst));
      kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_PMAPPOST, "");
      KlInstruction extra = *pc++;
      klcode_print_prefix(code, out, pc - 1, klcode_get_instname(extra));
      klcode_print_XI(out, extra);
      ko_printf(out, "jump to %u if not map", pc + KLINST_XI_GETI(extra) - code->code);
      return pc;
    }
    case KLOPCODE_PBMAP: {
      klcode_print_ABX(out, inst);
      ko_printf(out, "bind R%u to R%u, %u elements\n", KLINST_ABX_GETB(inst), KLINST_ABX_GETA(inst), KLINST_ABX_GETX(inst));
      kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_PMAPPOST, "");
      KlInstruction extra = *pc++;
      klcode_print_prefix(code, out, pc - 1, klcode_get_instname(extra));
      klcode_print_XI(out, extra);
      return pc;
    }
    case KLOPCODE_PMAPPOST: {
      klcode_print_XI(out, inst);
      ko_printf(out, "this instruction should not be printed in this way");
      return pc;
    }
    case KLOPCODE_PMOBJ:
    case KLOPCODE_PBOBJ: {
      klcode_print_ABX(out, inst);
      ko_printf(out, "bind R%u to R%u, %u fields", KLINST_ABX_GETB(inst), KLINST_ABX_GETA(inst), KLINST_ABX_GETX(inst));
      return pc;
    }
    case KLOPCODE_NEWOBJ: {
      klcode_print_AB(out, inst);
      return pc;
    }
    case KLOPCODE_ADJUSTARGS: {
      klcode_print_(out, inst);
      return pc;
    }
    case KLOPCODE_VFORPREP: {
      klcode_print_AI(out, inst);
      ko_printf(out, "jump to %u if finished", pc + KLINST_AI_GETI(inst) - code->code);
      return pc;
    }
    case KLOPCODE_VFORLOOP: {
      klcode_print_AI(out, inst);
      ko_printf(out, "jump to %u if continue", pc + KLINST_AI_GETI(inst) - code->code);
      return pc;
    }
    case KLOPCODE_IFORPREP: {
      klcode_print_AI(out, inst);
      ko_printf(out, "jump to %u if finished", pc + KLINST_AI_GETI(inst) - code->code);
      return pc;
    }
    case KLOPCODE_IFORLOOP: {
      klcode_print_AI(out, inst);
      ko_printf(out, "jump to %u if continue", pc + KLINST_AI_GETI(inst) - code->code);
      return pc;
    }
    case KLOPCODE_GFORLOOP: {
      klcode_print_AX(out, inst);
      ko_printf(out, "%u results", KLINST_AX_GETX(inst));
      /* following TRUEJMP just printed by next call of this function */
      return pc;
    }
    case KLOPCODE_ASYNC: {
      klcode_print_AB(out, inst);
      return pc;
    }
    case KLOPCODE_YIELD: {
      klcode_print_AXY(out, inst);
      size_t nres = KLINST_AXY_GETX(inst);
      size_t nwanted = KLINST_AXY_GETY(inst);
      nres == KLINST_VARRES ? ko_printf(out, "all results yielded, ") : ko_printf(out, "%u results yielded, ", nres);
      nwanted == KLINST_VARRES ? ko_printf(out, "all results wanted") : ko_printf(out, "%u results wanted", nwanted);
      return pc;
    }
    case KLOPCODE_VARARG: {
      klcode_print_AXY(out, inst);
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

static void klcode_print_reftbl(KlCode* code, Ko* out) {
  ko_printf(out, "%u references:\n", code->nref);
  for (size_t i = 0; i < code->nref; ++i)
    ko_printf(out, "reference %4zu: from %10s, index = %u\n", i, code->refinfo[i].on_stack ? "stack" : "ref table", (unsigned)code->refinfo[i].index);
}

typedef struct tagKlCodePrintInfo {
  size_t idx;
  struct tagKlCodePrintInfo* upper;
} KlCodePrintInfo;

static void klcode_print_printinfo(KlCodePrintInfo* printinfo, Ko* out) {
  if (printinfo) {
    klcode_print_printinfo(printinfo->upper, out);
    ko_printf(out, ":%zu", printinfo->idx);
  }
}

void klcode_print_function(KlCode* code, Ko* out, KlCodePrintInfo* printinfo) {
  KlInstruction* pc = code->code;
  KlInstruction* end = code->code + code->codelen;
  ko_printf(out, "function(top");
  klcode_print_printinfo(printinfo, out);
  ko_printf(out, "), %u registers\n", code->framesize);
  ko_printf(out, "%u instructions:\n", code->codelen);
  while (pc != end) {
    pc = klcode_print_instruction(code, out, pc);
    ko_putc(out, '\n');
  }
  ko_putc(out, '\n');
  ko_printf(out, "%u constants:\n", code->nconst);
  for (size_t i = 0; i < code->nconst; ++i) {
    ko_printf(out, "constant %4zu: ", i);
    klcode_print_constant(code, out, &code->constants[i]);
    ko_putc(out, '\n');
  }
  ko_putc(out, '\n');
  klcode_print_reftbl(code, out);
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

void klcode_print(KlCode* code, Ko* out) {
  ko_printf(out, "source file: %.*s\n", code->srcfile.length, klstrtbl_getstring(code->strtbl, code->srcfile.id));
  klcode_print_function(code, out, NULL);
}
