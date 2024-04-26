#include "klang/include/code/klcode.h"
#include "klang/include/misc/klutils.h"
#include "klang/include/vm/klinst.h"
#include <stdint.h>

static void klcode_print_prefix(Ko* out, KlFilePosition filepos, size_t pc, const char* name) {
  ko_printf(out, "(%4u, %4u) [%3u] %-16s", filepos.begin, filepos.end, pc, name);
}

static const const char* klcode_get_instname(KlInstruction inst) {
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
  ko_printf(out, "%4s %4s %4s %4s ", "", "", "", "");
}

static void klcode_print_A(Ko* out, KlInstruction inst) {
  ko_printf(out, "%4u %4s %4s %4s ", KLINST_ABC_GETA(inst), "", "", "");
}

static void klcode_print_AB(Ko* out, KlInstruction inst) {
  ko_printf(out, "%4u %4u %4s %4s ", KLINST_ABC_GETA(inst), KLINST_ABC_GETB(inst), "", "");
}

static void klcode_print_AX(Ko* out, KlInstruction inst) {
  ko_printf(out, "%4u %4u %4s %4s ", KLINST_AX_GETA(inst), KLINST_AX_GETX(inst), "", "");
}

static void klcode_print_ABC(Ko* out, KlInstruction inst) {
  ko_printf(out, "%4u %4u %4u %4s ", KLINST_ABC_GETA(inst), KLINST_ABC_GETB(inst), KLINST_ABC_GETC(inst), "");
}

static void klcode_print_ABI(Ko* out, KlInstruction inst) {
  ko_printf(out, "%4u %4u %4d %4s ", KLINST_ABI_GETA(inst), KLINST_ABI_GETB(inst), KLINST_ABI_GETI(inst), "");
}

static void klcode_print_AIJ(Ko* out, KlInstruction inst) {
  ko_printf(out, "%4u %4d %4d %4s ", KLINST_AIJ_GETA(inst), KLINST_AIJ_GETI(inst), KLINST_AIJ_GETJ(inst), "");
}

static void klcode_print_AI(Ko* out, KlInstruction inst) {
  ko_printf(out, "%4u %4d %4s %4s ", KLINST_AI_GETA(inst), KLINST_AI_GETI(inst), "", "");
}

static void klcode_print_I(Ko* out, KlInstruction inst) {
  ko_printf(out, "%4u %4s %4s %4s ", KLINST_I_GETI(inst), "", "", "");
}

static void klcode_print_constant(KlCode* code, Ko* out, KlConstant* constant) {
  switch (constant->type) {
    case KL_INT: {
      ko_printf(out, "%zd", constant->intval);
      break;
    }
    case KL_FLOAT: {
      ko_printf(out, "%lf", constant->floatval);
      break;
    }
    case KL_STRING: {
      ko_printf(out, "%*.s", constant->string.length, klstrtbl_getstring(code->strtbl, constant->string.id));
      break;
    }
    case KL_BOOL: {
      ko_printf(out, "%s", constant->boolval ? "true" : "false");
      break;
    }
    default: {
      kl_assert(false, "impossible constant type");
    }
  }
}

static KlInstruction* klcode_print_instruction(KlCode* code, Ko* out, KlInstruction* pc) {
  KlInstruction inst = *pc++;
  klcode_print_prefix(out, code->lineinfo[pc - code->code], pc - code->code, klcode_get_instname(inst));
  uint8_t opcode = KLINST_GET_OPCODE(inst);
  switch (opcode) {
    case KLOPCODE_MOVE: {
      klcode_print_AB(out, inst);
      return pc;
    }
    case KLOPCODE_MULTIMOVE: {
      klcode_print_ABC(out, inst);
      if (KLINST_ABX_GETX(inst) == KLINST_VARRES)
        ko_printf(out, "move all");
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
      return pc;
    }
    case KLOPCODE_SUBI: {
      klcode_print_ABI(out, inst);
      return pc;
    }
    case KLOPCODE_MULI: {
      klcode_print_ABI(out, inst);
      return pc;
    }
    case KLOPCODE_DIVI: {
      klcode_print_ABI(out, inst);
      return pc;
    }
    case KLOPCODE_MODI: {
      klcode_print_ABI(out, inst);
      return pc;
    }
    case KLOPCODE_IDIVI: {
      klcode_print_ABI(out, inst);
      return pc;
    }
    case KLOPCODE_ADDC: {
      klcode_print_ABC(out, inst);
      klcode_print_constant(code, out, &code->constants[KLINST_ABX_GETX(inst)]);
      return pc;
    }
    case KLOPCODE_SUBC: {
      klcode_print_ABC(out, inst);
      klcode_print_constant(code, out, &code->constants[KLINST_ABX_GETX(inst)]);
      return pc;
    }
    case KLOPCODE_MULC: {
      klcode_print_ABC(out, inst);
      klcode_print_constant(code, out, &code->constants[KLINST_ABX_GETX(inst)]);
      return pc;
    }
    case KLOPCODE_DIVC: {
      klcode_print_ABC(out, inst);
      klcode_print_constant(code, out, &code->constants[KLINST_ABX_GETX(inst)]);
      return pc;
    }
    case KLOPCODE_MODC: {
      klcode_print_ABC(out, inst);
      klcode_print_constant(code, out, &code->constants[KLINST_ABX_GETX(inst)]);
      return pc;
    }
    case KLOPCODE_IDIVC: {
      klcode_print_ABC(out, inst);
      klcode_print_constant(code, out, &code->constants[KLINST_ABX_GETX(inst)]);
      return pc;
    }
    case KLOPCODE_NEG: {
      klcode_print_A(out, inst);
      return pc;
    }
    case KLOPCODE_SCALL: {
      klcode_print_ABC(out, inst);
      size_t narg = KLINST_AXY_GETX(inst);
      size_t nret = KLINST_AXY_GETY(inst);
      narg == KLINST_VARRES ? ko_printf(out, "all arguments") : ko_printf(out, "%u arguments", narg);
      nret == KLINST_VARRES ? ko_printf(out, " all results") : ko_printf(out, " %u results", nret);
      return pc;
    }
    case KLOPCODE_CALL: {
      klcode_print_A(out, inst);
      ko_printf(out, "callable object at %u\n", KLINST_AXY_GETA(inst));
      KlInstruction extra = *pc++;

      klcode_print_prefix(out, code->lineinfo[pc - code->code], pc - code->code, "CALL EXTRA");
      klcode_print_ABC(out, extra);
      size_t narg = KLINST_XYZ_GETX(extra);
      size_t nret = KLINST_XYZ_GETY(extra);
      size_t target = KLINST_XYZ_GETZ(extra);
      narg == KLINST_VARRES ? ko_printf(out, "all arguments") : ko_printf(out, "%u arguments", narg);
      nret == KLINST_VARRES ? ko_printf(out, " all results") : ko_printf(out, " %u results", nret);
      ko_printf(out, "target at %u", target);
      return pc;
    }
    case KLOPCODE_METHOD: {
      klcode_print_AX(out, inst);
      ko_printf(out, "object at %u, method name: ", KLINST_AX_GETA(inst));
      klcode_print_constant(code, out, &code->constants[KLINST_AX_GETX(inst)]);
      ko_putc(out, '\n');
      KlInstruction extra = *pc++;

      klcode_print_prefix(out, code->lineinfo[pc - code->code], pc - code->code, "METHOD EXTRA");
      klcode_print_ABC(out, extra);
      size_t narg = KLINST_XYZ_GETX(extra);
      size_t nret = KLINST_XYZ_GETY(extra);
      size_t target = KLINST_XYZ_GETZ(extra);
      narg == KLINST_VARRES ? ko_printf(out, "all arguments") : ko_printf(out, "%u arguments", narg);
      nret == KLINST_VARRES ? ko_printf(out, " all results") : ko_printf(out, " %u results", nret);
      ko_printf(out, "target at %u", target);
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
      ko_printf(out, "%zd", KLINST_AI_GETI(inst));
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
      klcode_print_constant(code, out, &code->constants[KLINST_AX_GETX(inst)]);
      return pc;
    }
    case KLOPCODE_STOREREF: {
      klcode_print_AX(out, inst);
      return pc;
    }
    case KLOPCODE_STOREGLOBAL: {
      klcode_print_AX(out, inst);
      klcode_print_constant(code, out, &code->constants[KLINST_AX_GETX(inst)]);
      return pc;
    }
    case KLOPCODE_MKMAP: {
      /* this instruction tells us current stack top */
      size_t stktop = KLINST_ABX_GETB(inst);
      size_t capacity = KLINST_ABX_GETX(inst);
      klcode_print_ABC(out, inst);
      ko_printf(out, "capacity: %zu, stack top: %u", klbit(capacity), stktop);
      return pc;
    }
    case KLOPCODE_MKARRAY: {
      klcode_print_ABC(out, inst);
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
      klcode_print_ABC(out, inst);
      size_t nelem = KLINST_ABX_GETX(inst);
      nelem == KLINST_VARRES ? ko_printf(out, "many elements") : ko_printf(out, "%u elements", nelem);
      return pc;
    }
    case KLOPCODE_MKCLASS: {
      /* this instruction tells us current stack top */
      size_t stktop = KLINST_ABTX_GETB(inst);
      size_t capacity = KLINST_ABTX_GETX(inst);
      klcode_print_ABC(out, inst);
      ko_printf(out, "capacity: %zu, stack top: %u", klbit(capacity), stktop);
      if (KLINST_ABTX_GETT(inst))
        ko_printf(out, " parent at ", stktop);
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
      klcode_print_constant(code, out, &code->constants[KLINST_ABC_GETC(inst)]);
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
      klcode_print_constant(code, out, &code->constants[KLINST_ABC_GETC(inst)]);
      ko_printf(out, "\n = R%u", KLINST_ABC_GETA(inst));
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
      klcode_print_constant(code, out, &code->constants[KLINST_AXY_GETX(inst)]);
      return pc;
    }
    case KLOPCODE_REFSETFIELDR: {
      KlValue* val = stkbase + KLINST_ABX_GETA(inst);
      KlValue* key = stkbase + KLINST_ABX_GETB(inst);
      KlValue* dotable = klref_getval(klkclosure_getref(closure, KLINST_ABX_GETX(inst)));
      KlException exception = klexec_setfieldgeneric(state, dotable, key, val);
      if (kl_unlikely(exception)) {
        klexec_savepc(callinfo, pc);
        return exception;
      }
      break;
    }
    case KLOPCODE_REFSETFIELDC: {
      KlValue* val = stkbase + KLINST_AXY_GETA(inst);
      KlValue* key = constants + KLINST_AXY_GETX(inst);
      KlValue* dotable = klref_getval(klkclosure_getref(closure, KLINST_AXY_GETY(inst)));
      KlException exception = klexec_setfieldgeneric(state, dotable, key, val);
      if (kl_unlikely(exception)) {
        klexec_savepc(callinfo, pc);
        return exception;
      }
      break;
    }
    case KLOPCODE_NEWLOCAL: {
      KlValue* classval = stkbase + KLINST_AX_GETA(inst);
      KlValue* fieldname = constants + KLINST_AX_GETX(inst);
      kl_assert(klvalue_checktype(classval, KL_CLASS), "NEWLOCAL should applied to a class");
      kl_assert(klvalue_checktype(fieldname, KL_STRING), "expected string to index field");

      KlString* keystr = klvalue_getobj(fieldname, KlString*);
      KlClass* klclass = klvalue_getobj(classval, KlClass*);
      klexec_savestate(callinfo->top);  /* add new field */
      KlException exception = klclass_newlocal(klclass, klstate_getmm(state), keystr);
      if (kl_unlikely(exception))
        return klexec_handle_newlocal_exception(state, exception, keystr);
      break;
    }
    case KLOPCODE_LOADFALSESKIP: {
      KlValue* a = stkbase + KLINST_A_GETA(inst);
      klvalue_setbool(a, KL_FALSE);
      ++pc;
      break;
    }
    case KLOPCODE_TESTSET: {
      KlValue* a = stkbase + KLINST_ABC_GETA(inst);
      KlValue* b = stkbase + KLINST_ABC_GETB(inst);
      kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
      KlInstruction extra = *pc++;
      bool cond = KLINST_XI_GETX(extra);
      if (klexec_satisfy(b, cond)) {
        klvalue_setvalue(a, b);
        pc += KLINST_XI_GETI(extra);
      }
      break;
    }
    case KLOPCODE_TRUEJMP: {
      KlValue* a = stkbase + KLINST_ABC_GETA(inst);
      int offset = KLINST_XI_GETI(inst);
      if (klexec_satisfy(a, KL_TRUE)) pc += offset;
      break;
    }
    case KLOPCODE_FALSEJMP: {
      KlValue* a = stkbase + KLINST_ABC_GETA(inst);
      int offset = KLINST_XI_GETI(inst);
      if (klexec_satisfy(a, KL_FALSE)) pc += offset;
      break;
    }
    case KLOPCODE_JMP: {
      int offset = KLINST_I_GETI(inst);
      pc += offset;
      break;
    }
    case KLOPCODE_CONDJMP: {
      /* This instruction must follow a comparison instruction.
         * This instruction can only be executed if the preceding comparison
         * instruction invoked the object's comparison method and the method
         * is not C function, and the method has returned. In this case, the
         * comparison result is stored at 'callinfo->top'.
         */
      bool cond = KLINST_XI_GETX(inst);
      KlValue* val = callinfo->top;
      if (klexec_satisfy(val, cond))
        pc += KLINST_XI_GETI(inst);
      break;
    }
    case KLOPCODE_CLOSEJMP: {
      KlValue* bound = stkbase + KLINST_XI_GETX(inst);
      klreflist_close(&state->reflist, bound, klstate_getmm(state));
      pc += KLINST_XI_GETI(inst);
      break;
    }
    case KLOPCODE_HASFIELD: {
      KlValue* a = stkbase + KLINST_AX_GETA(inst);
      KlValue* field = constants + KLINST_AX_GETX(inst);
      kl_assert(klvalue_checktype(field, KL_STRING), "");
      kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
      KlInstruction extra = *pc++;
      bool cond = KLINST_XI_GETX(extra);
      int offset = KLINST_XI_GETI(extra);
      if (!klvalue_checktype(klexec_getfield(state, a, klvalue_getobj(field, KlString*)), KL_NIL)) {
        if (cond) pc += offset;
      } else {
        if (!cond) pc += offset;
      }
      break;
    }
    case KLOPCODE_IS: {
      KlValue* a = stkbase + KLINST_ABC_GETA(inst);
      KlValue* b = stkbase + KLINST_ABC_GETB(inst);
      kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
      KlInstruction extra = *pc++;
      bool cond = KLINST_XI_GETX(extra);
      int offset = KLINST_XI_GETI(extra);
      if (klvalue_sametype(a, b) && klvalue_sameinstance(a, b)) {
        if (cond) pc += offset;
      } else {
        if (!cond) pc += offset;
      }
      break;
    }
    case KLOPCODE_EQ: {
      KlValue* a = stkbase + KLINST_ABC_GETA(inst);
      KlValue* b = stkbase + KLINST_ABC_GETB(inst);
      kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
      KlInstruction extra = *pc++;
      bool cond = KLINST_XI_GETX(extra);
      int offset = KLINST_XI_GETI(extra);
      klexec_bequal(a, b, offset, cond);
      break;
    }
    case KLOPCODE_NE: {
      KlValue* a = stkbase + KLINST_ABC_GETA(inst);
      KlValue* b = stkbase + KLINST_ABC_GETB(inst);
      kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
      KlInstruction extra = *pc++;
      bool cond = KLINST_XI_GETX(extra);
      int offset = KLINST_XI_GETI(extra);
      klexec_bnequal(a, b, offset, cond);
      break;
    }
    case KLOPCODE_LT: {
      KlValue* a = stkbase + KLINST_ABC_GETA(inst);
      KlValue* b = stkbase + KLINST_ABC_GETB(inst);
      kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
      KlInstruction condjmp = *pc++;
      bool cond = KLINST_XI_GETX(condjmp);
      int offset = KLINST_XI_GETI(condjmp);
      klexec_order(lt, a, b, offset, cond);
      break;
    }
    case KLOPCODE_GT: {
      KlValue* a = stkbase + KLINST_ABC_GETA(inst);
      KlValue* b = stkbase + KLINST_ABC_GETB(inst);
      kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
      KlInstruction condjmp = *pc++;
      bool cond = KLINST_XI_GETX(condjmp);
      int offset = KLINST_XI_GETI(condjmp);
      klexec_order(gt, a, b, offset, cond);
      break;
    }
    case KLOPCODE_LE: {
      KlValue* a = stkbase + KLINST_ABC_GETA(inst);
      KlValue* b = stkbase + KLINST_ABC_GETB(inst);
      kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
      KlInstruction condjmp = *pc++;
      bool cond = KLINST_XI_GETX(condjmp);
      int offset = KLINST_XI_GETI(condjmp);
      klexec_order(le, a, b, offset, cond);
      break;
    }
    case KLOPCODE_GE: {
      KlValue* a = stkbase + KLINST_ABC_GETA(inst);
      KlValue* b = stkbase + KLINST_ABC_GETB(inst);
      kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
      KlInstruction condjmp = *pc++;
      bool cond = KLINST_XI_GETX(condjmp);
      int offset = KLINST_XI_GETI(condjmp);
      klexec_order(ge, a, b, offset, cond);
      break;
    }
    case KLOPCODE_EQC: {
      KlValue* a = stkbase + KLINST_AX_GETA(inst);
      KlValue* b = constants + KLINST_AX_GETX(inst);
      kl_assert(klvalue_canrawequal(b), "something wrong in EQC");
      kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
      KlInstruction condjmp = *pc++;
      int offset = KLINST_XI_GETI(condjmp);
      if (klvalue_equal(a, b))
        pc += offset;
      break;
    }
    case KLOPCODE_NEC: {
      KlValue* a = stkbase + KLINST_AX_GETA(inst);
      KlValue* b = constants + KLINST_AX_GETX(inst);
      kl_assert(klvalue_canrawequal(b), "something wrong in NEC");
      kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
      KlInstruction condjmp = *pc++;
      int offset = KLINST_XI_GETI(condjmp);
      if (!klvalue_equal(a, b))
        pc += offset;
      break;
    }
    case KLOPCODE_LTC: {
      KlValue* a = stkbase + KLINST_AX_GETA(inst);
      KlValue* b = constants + KLINST_AX_GETX(inst);
      kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
      KlInstruction condjmp = *pc++;
      bool cond = KLINST_XI_GETX(condjmp);
      int offset = KLINST_XI_GETI(condjmp);
      klexec_order(lt, a, b, offset, cond);
      break;
    }
    case KLOPCODE_GTC: {
      KlValue* a = stkbase + KLINST_AX_GETA(inst);
      KlValue* b = constants + KLINST_AX_GETX(inst);
      kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
      KlInstruction condjmp = *pc++;
      bool cond = KLINST_XI_GETX(condjmp);
      int offset = KLINST_XI_GETI(condjmp);
      klexec_order(gt, a, b, offset, cond);
      break;
    }
    case KLOPCODE_LEC: {
      KlValue* a = stkbase + KLINST_AX_GETA(inst);
      KlValue* b = constants + KLINST_AX_GETX(inst);
      kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
      KlInstruction condjmp = *pc++;
      bool cond = KLINST_XI_GETX(condjmp);
      int offset = KLINST_XI_GETI(condjmp);
      klexec_order(le, a, b, offset, cond);
      break;
    }
    case KLOPCODE_GEC: {
      KlValue* a = stkbase + KLINST_AX_GETA(inst);
      KlValue* b = constants + KLINST_AX_GETX(inst);
      kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
      KlInstruction condjmp = *pc++;
      bool cond = KLINST_XI_GETX(condjmp);
      int offset = KLINST_XI_GETI(condjmp);
      klexec_order(ge, a, b, offset, cond);
      break;
    }
    case KLOPCODE_EQI: {
      KlValue* a = stkbase + KLINST_AI_GETA(inst);
      int imm = KLINST_AI_GETI(inst);
      kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
      KlInstruction condjmp = *pc++;
      int offset = KLINST_XI_GETI(condjmp);
      if (klvalue_checktype(a, KL_INT) && klvalue_getint(a) == imm)
        pc += offset;
      break;
    }
    case KLOPCODE_NEI: {
      KlValue* a = stkbase + KLINST_AI_GETA(inst);
      KlInt imm = KLINST_AI_GETI(inst);
      kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
      KlInstruction condjmp = *pc++;
      int offset = KLINST_XI_GETI(condjmp);
      if (!klvalue_checktype(a, KL_INT) || klvalue_getint(a) != imm)
        pc += offset;
      break;
    }
    case KLOPCODE_LTI: {
      KlValue* a = stkbase + KLINST_AI_GETA(inst);
      KlInt imm = KLINST_AI_GETI(inst);
      kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
      KlInstruction condjmp = *pc++;
      bool cond = KLINST_XI_GETX(condjmp);
      int offset = KLINST_XI_GETI(condjmp);
      klexec_orderi(lt, a, imm, offset, cond);
      break;
    }
    case KLOPCODE_GTI: {
      KlValue* a = stkbase + KLINST_AI_GETA(inst);
      KlInt imm = KLINST_AI_GETI(inst);
      kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
      KlInstruction condjmp = *pc++;
      bool cond = KLINST_XI_GETX(condjmp);
      int offset = KLINST_XI_GETI(condjmp);
      klexec_orderi(gt, a, imm, offset, cond);
      break;
    }
    case KLOPCODE_LEI: {
      KlValue* a = stkbase + KLINST_AI_GETA(inst);
      KlInt imm = KLINST_AI_GETI(inst);
      kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
      KlInstruction condjmp = *pc++;
      bool cond = KLINST_XI_GETX(condjmp);
      int offset = KLINST_XI_GETI(condjmp);
      klexec_orderi(le, a, imm, offset, cond);
      break;
    }
    case KLOPCODE_GEI: {
      KlValue* a = stkbase + KLINST_AI_GETA(inst);
      KlInt imm = KLINST_AI_GETI(inst);
      kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
      KlInstruction condjmp = *pc++;
      bool cond = KLINST_XI_GETX(condjmp);
      int offset = KLINST_XI_GETI(condjmp);
      klexec_orderi(ge, a, imm, offset, cond);
      break;
    }
    case KLOPCODE_MATCH: {
      KlValue* a = stkbase + KLINST_AI_GETA(inst);
      KlValue* b = constants + KLINST_AX_GETX(inst);
      if (kl_unlikely(!(klvalue_sametype(a, b) && klvalue_sameinstance(a, b)))) {
        klexec_savestate(callinfo->top);
        return klstate_throw(state, KL_E_DISMATCH, "pattern dismatch");
      }
      break;
    }
    case KLOPCODE_PMARR:
    case KLOPCODE_PBARR: {
      kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_EXTRA, "");
      KlValue* b = stkbase + KLINST_ABX_GETB(inst);
      if (!klvalue_checktype(b, KL_ARRAY)) {
        if (kl_unlikely(opcode == KLOPCODE_PBARR)) {  /* is pattern binding? */
          klexec_savestate(callinfo->top);
          return klstate_throw(state, KL_E_TYPE, "pattern binding: not an array");
        }
        /* else jump out */
        KlInstruction extra = *pc++;
        pc += KLINST_XI_GETI(extra);
        break;
      }
      KlInstruction extra = *pc++;
      /* else is array */
      KlValue* a = stkbase + KLINST_ABX_GETA(inst);
      KlArray* array = klvalue_getobj(b, KlArray*);
      size_t nfront = KLINST_ABX_GETX(inst);
      size_t arrsize = klarray_size(array);
      KlValue* end = a + (arrsize < nfront ? arrsize : nfront);
      KlValue* begin = a;
      for (KlValue* itr = klarray_iter_begin(array); begin != end; ++itr)
        klvalue_setvalue(begin++, itr);
      while (begin != a + nfront) klvalue_setnil(begin++); /* complete missing values */
      size_t nback = KLINST_XI_GETX(extra);
      begin += nback;
      kl_assert(begin == a + nfront + nback, "");
      end = begin - (arrsize < nback ? arrsize : nback);
      for (KlArrayIter itr = klarray_top(array); begin != end; --itr)
        klvalue_setvalue(begin--, klarray_iter_get(array, itr));
      while (begin != a + nfront) klvalue_setnil(begin--); /* complete missing values */
      break;
    }
    case KLOPCODE_PMTUP:
    case KLOPCODE_PBTUP: {
      KlValue* b = stkbase + KLINST_ABX_GETB(inst);
      size_t nwanted = KLINST_ABX_GETX(inst);
      KlArray* array = klvalue_getobj(b, KlArray*);
      if (!klvalue_checktype(b, KL_ARRAY) || klarray_size(array) != nwanted) {
        if (kl_unlikely(opcode == KLOPCODE_PBTUP)) {  /* is pattern binding? */
          klexec_savestate(callinfo->top);
          return klstate_throw(state, KL_E_TYPE, "pattern binding: not an array with %zd elements", nwanted);
        }
        /* else jump out */
        kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_EXTRA, "");
        KlInstruction extra = *pc++;
        pc += KLINST_XI_GETI(extra);
        break;
      }
      /* else is array with exactly 'nwanted' elements */
      if (opcode == KLOPCODE_PMTUP) /* is pattern binding? */
        ++pc; /* skip extra instruction */
      KlValue* a = stkbase + KLINST_ABX_GETA(inst);
      KlValue* end = klarray_iter_end(array);
      for (KlValue* itr = klarray_iter_begin(array); itr != end; ++itr)
        klvalue_setvalue(a++, itr);
      break;
    }
    case KLOPCODE_PMMAP:
    case KLOPCODE_PBMAP: {
      KlValue* b = stkbase + KLINST_ABX_GETB(inst);
      if (!klvalue_checktype(b, KL_MAP)) {
        if (kl_unlikely(opcode == KLOPCODE_PBMAP)) {  /* is pattern binding? */
          klexec_savestate(callinfo->top);
          return klstate_throw(state, KL_E_TYPE, "pattern binding: not an map");
        }
        /* else jump out, pattern matching instruction must be followed by extra information */
        kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_EXTRA, "");
        KlInstruction extra = *pc++;
        pc += KLINST_XI_GETI(extra);
        break;
      }
      ++pc; /* skip pmmappost */
      KlValue* a = stkbase + KLINST_ABX_GETA(inst);
      size_t nwanted = KLINST_ABX_GETX(inst);
      KlMap* map = klvalue_getobj(b, KlMap*);
      KlValue* key = b + 1;
      /* 'b' may be overwritten while do pattern binding,
         * so store 'b' to another position on stack that will not be overwitten
         * so that 'b' would not be collected by garbage collector */
      kl_assert(b + nwanted + 2 < klstack_size(klstate_stack(state)) + klstack_raw(klstate_stack(state)), "compiler error");
      klvalue_setvalue(b + nwanted + 1, b);
      for (size_t i = 0; i < nwanted; ++i) {
        if (kl_likely(klvalue_canrawequal(key + i))) {
          KlMapIter itr = klmap_search(map, key + i);
          itr ? klvalue_setvalue(a + i, &itr->value) : klvalue_setnil(a + i);
        } else {
          klvalue_setint(b + nwanted + 2, i); /* save current index for pmappost */
          klexec_savestate(callinfo->top);
          KlException exception = klexec_doindexmethod(state, a + i, b + nwanted + 1, key + i);
          if (kl_likely(callinfo != state->callinfo)) { /* is a klang call ? */
            KlValue* newbase = state->callinfo->base;
            klexec_updateglobal(newbase);
            --pc; /* do pmmappost instruction */
            break;  /* break to execute new function */
          } else {
            if (kl_unlikely(exception)) return exception;
            /* C function or C closure */
            /* stack may have grown. restore stkbase. */
            stkbase = callinfo->base;
          }
        }
      }
      break;
    }
    case KLOPCODE_PMAPPOST: {
      /* This instruction must follow map pattern binding or matching instruction.
         * This instruction can only be executed if the preceding instruction
         * call the index method of map and the method is not C function, and the
         * method has returned. This instruction will complete things not done by
         * previous instruction.
         */
      KlInstruction previnst = *(pc - 2);
      KlValue* b = stkbase + KLINST_ABX_GETB(previnst);
      KlValue* a = stkbase + KLINST_ABX_GETA(previnst);
      size_t nwanted = KLINST_ABX_GETX(previnst);
      KlMap* map = klvalue_getobj(b + nwanted + 1, KlMap*);
      KlValue* key = b + 1;
      kl_assert(klvalue_checktype(b + nwanted + 1, KL_MAP), "");
      kl_assert(klvalue_checktype(b + nwanted + 2, KL_INT), "");
      for (size_t i = klvalue_getint(b + nwanted + 2); i < nwanted; ++i) {
        if (kl_likely(klvalue_canrawequal(key + i))) {
          KlMapIter itr = klmap_search(map, key + i);
          itr ? klvalue_setvalue(a + i, &itr->value) : klvalue_setnil(a + i);
        } else {
          klvalue_setint(b + nwanted + 2, i); /* save current index */
          klexec_savestate(callinfo->top);
          KlException exception = klexec_doindexmethod(state, a + i, b + nwanted + 1, key + i);
          if (kl_likely(callinfo != state->callinfo)) { /* is a klang call ? */
            KlValue* newbase = state->callinfo->base;
            klexec_updateglobal(newbase);
            --pc;   /* this instruction should continue after the new function returned */
            break;  /* break to execute new function */
          } else {
            if (kl_unlikely(exception)) return exception;
            /* C function or C closure */
            /* stack may have grown. restore stkbase. */
            stkbase = callinfo->base;
          }
        }
      }
      break;
    }
    case KLOPCODE_PMOBJ:
    case KLOPCODE_PBOBJ: {
      KlValue* obj = stkbase + KLINST_ABX_GETB(inst);
      KlValue* a = stkbase + KLINST_ABX_GETA(inst);
      size_t nwanted = KLINST_ABX_GETX(inst);
      KlValue* field = obj + 1;
      for (size_t i = 0; i < nwanted; ++i) {
        kl_assert(klvalue_checktype(field + i, KL_STRING), "");
        KlString* fieldname = klvalue_getobj(field + i, KlString*);
        klvalue_setvalue(a + i, klexec_getfield(state, obj, fieldname));
      }
      break;
    }
    case KLOPCODE_NEWOBJ: {
      KlValue* klclass = stkbase + KLINST_ABC_GETB(inst);
      if (kl_unlikely(!klvalue_checktype(klclass, KL_CLASS)))
        return klstate_throw(state, KL_E_TYPE, "%s is not a class", klvalue_typename(klvalue_gettype(klclass)));
      klexec_savestate(callinfo->top);
      KlException exception = klclass_new_object(klvalue_getobj(klclass, KlClass*), klstate_getmm(state), stkbase + KLINST_ABC_GETA(inst));
      if (kl_unlikely(exception))
        return klexec_handle_newobject_exception(state, exception);
      break;
    }
    case KLOPCODE_ADJUSTARGS: {
      size_t narg = klstate_stktop(state) - stkbase;
      size_t nparam = klkfunc_nparam(closure->kfunc);
      kl_assert(narg >= nparam, "something wrong in callprepare");
      /* a closure having variable arguments needs 'narg'. */
      callinfo->narg = narg;
      if (narg > nparam) {
        /* it's not necessary to grow stack here.
           * callprepare ensures enough stack frame size.
           */
        KlValue* fixed = stkbase;
        KlValue* stktop = klstate_stktop(state);
        while (nparam--)   /* move fixed arguments to top */
          klvalue_setvalue(stktop++, fixed++);
        callinfo->top += narg;
        callinfo->base += narg;
        callinfo->retoff -= narg;
        stkbase += narg;
        klstack_set_top(klstate_stack(state), stktop);
      }
      break;
    }
    case KLOPCODE_VFORPREP: {
      KlValue* a = stkbase + KLINST_AI_GETA(inst);
      kl_assert(klvalue_checktype(a, KL_INT), "compiler error");
      KlInt nextra = callinfo->narg - klkfunc_nparam(closure->kfunc);
      KlInt step = klvalue_getint(a);
      kl_assert(step > 0, "compiler error");
      if (nextra != 0) {
        KlValue* varpos = a + 2;
        KlValue* argpos = stkbase - nextra;
        size_t nvalid = step > nextra ? nextra : step;
        klvalue_setint(a + 1, nextra - nvalid);  /* set index for next iteration */
        while (nvalid--)
          klvalue_setvalue(varpos++, argpos++);
        while (step-- > nextra)
          klvalue_setnil(varpos++);
      } else {
        pc += KLINST_AI_GETI(inst);
      }
      break;
    }
    case KLOPCODE_VFORLOOP: {
      KlValue* a = stkbase + KLINST_AI_GETA(inst);
      kl_assert(klvalue_checktype(a, KL_INT), "");
      KlInt step = klvalue_getint(a);
      KlInt idx = klvalue_getint(a + 1);
      if (idx != 0) {
        KlValue* varpos = a + 2;
        KlValue* argpos = stkbase - idx;
        size_t nvalid = step > idx ? idx : step;
        klvalue_setint(a + 1, idx - nvalid);
        while (nvalid--)
          klvalue_setvalue(varpos++, argpos++);
        while (step-- > idx)
          klvalue_setnil(varpos++);
        pc += KLINST_AI_GETI(inst);
      }
      break;
    }
    case KLOPCODE_IFORPREP: {
      KlValue* a = stkbase + KLINST_AI_GETA(inst);
      int offset = KLINST_AI_GETI(inst);
      klexec_savestate(a + 3);
      KlException exception = klexec_iforprep(state, a, offset);
      if (kl_unlikely(exception)) return exception;
      pc = callinfo->savedpc; /* pc may be changed by klexec_iforprep() */
      break;
    }
    case KLOPCODE_IFORLOOP: {
      KlValue* a = stkbase + KLINST_AI_GETA(inst);
      kl_assert(klvalue_checktype(a, KL_INT) && klvalue_checktype(a, KL_INT) && klvalue_checktype(a, KL_INT), "");
      KlInt i = klvalue_getint(a);
      KlInt end = klvalue_getint(a + 1);
      KlInt step = klvalue_getint(a + 2);
      i += step;
      if (i != end) {
        pc += KLINST_AI_GETI(inst);
        klvalue_setint(a, i);
      }
      break;
    }
    case KLOPCODE_GFORLOOP: {
      KlValue* a = stkbase + KLINST_AX_GETA(inst);
      size_t nret = KLINST_AX_GETX(inst);
      KlValue* argbase = a + 1;
      klexec_savestate(argbase + nret);
      KlCallInfo* newci = klexec_new_callinfo(state, nret, 0);
      if (kl_unlikely(!newci))
        return klstate_throw(state, KL_E_OOM, "out of memory when do generic for loop");
      KlException exception = klexec_callprepare(state, a, nret, NULL);
      if (callinfo != state->callinfo) { /* is a klang call ? */
        KlValue* newbase = state->callinfo->base;
        klexec_updateglobal(newbase);
      } else {
        if (kl_unlikely(exception)) return exception;
        /* C function or C closure */
        /* stack may have grown. restore stkbase. */
        stkbase = callinfo->base;
        KlInstruction jmp = *pc++;
        kl_assert(KLINST_GET_OPCODE(jmp) == KLOPCODE_TRUEJMP && KLINST_AI_GETA(jmp) == KLINST_AX_GETA(inst) + 1, "");
        KlValue* testval = stkbase + KLINST_AX_GETA(inst) + 1;
        if (kl_likely(klexec_satisfy(testval, KL_TRUE)))
          pc += KLINST_AI_GETI(jmp);
      }
      break;
    }
    case KLOPCODE_ASYNC: {
      KlValue* f = stkbase + KLINST_ABC_GETA(inst);
      klexec_savestate(callinfo->top);
      if (kl_unlikely(klvalue_checktype(f, KL_KCLOSURE))) {
        return klstate_throw(state, KL_E_TYPE,
                             "async should be applied to a klang closure, got '%s'",
                             klvalue_typename(klvalue_gettype(f)));
      }
      KlState* costate = klco_create(state, klvalue_getobj(f, KlKClosure*));
      if (kl_unlikely(!costate)) return klstate_throw(state, KL_E_OOM, "out of memory when creating a coroutine");
      KlValue* a = stkbase + KLINST_ABC_GETA(inst);
      klvalue_setobj(a, costate, KL_COROUTINE);
      break;
    }
    case KLOPCODE_YIELD: {
      if (kl_unlikely(!klco_valid(&state->coinfo)))   /* is this 'state' a valid coroutine? */
        return klstate_throw(state, KL_E_INVLD, "can not yield from outside a coroutine");
      KlValue* first = stkbase + KLINST_AXY_GETA(inst);
      size_t nres = KLINST_AXY_GETX(inst);
      if (nres == KLINST_VARRES)
        nres = klstate_stktop(state) - first;
      size_t nwanted = KLINST_AXY_GETY(inst);
      klexec_savestate(first + nres);
      klco_yield(&state->coinfo, first, nres, nwanted);
      return KL_E_NONE;
    }
    case KLOPCODE_VARARG: {
      size_t nwanted = KLINST_AXY_GETX(inst);
      size_t nvarg = callinfo->narg - klkfunc_nparam(closure->kfunc);
      if (nwanted == KLINST_VARRES)
        nwanted = nvarg;
      klexec_savestate(callinfo->top);
      if (kl_unlikely(klstate_checkframe(state, nwanted)))
        return klstate_throw(state, KL_E_OOM, "out of memory when copy '...' to stack");
      size_t ncopy = nwanted < nvarg ? nwanted : nvarg;
      KlValue* a = stkbase + KLINST_AXY_GETA(inst);
      KlValue* b = stkbase - nvarg;
      while (ncopy--)
        klvalue_setvalue(a++, b++);
      if (nwanted > nvarg)
        klexec_setnils(a, nwanted - nvarg);
      if (KLINST_AXY_GETX(inst) == KLINST_VARRES)
        klstack_set_top(klstate_stack(state), a);
      break;
    }
    default: {
      kl_assert(false, "control flow should not reach here");
      break;
    }
  }
}

void klcode_print(KlCode* code, Ko* out) {
  KlInstruction* pc = code->code;
  while (true) {
    KlInstruction inst = *pc++;
    uint8_t opcode = KLINST_GET_OPCODE(inst);
    switch (opcode) {
      case KLOPCODE_MOVE: {

        break;
      }
      case KLOPCODE_MULTIMOVE: {
        KlValue* a = stkbase + KLINST_ABX_GETA(inst);
        KlValue* b = stkbase + KLINST_ABX_GETB(inst);
        size_t nmove = KLINST_ABX_GETX(inst);
        if (nmove == KLINST_VARRES) {
          nmove = klstate_stktop(state) - b;
          klstack_set_top(klstate_stack(state), a + nmove);
        }
        if (a <= b) {
          while (nmove--)
            klvalue_setvalue(a++, b++);
        } else {
          a += nmove;
          b += nmove;
          while (nmove--)
            klvalue_setvalue(--a, --b);
        }
        break;
      }
      case KLOPCODE_ADD: {
        KlValue* a = stkbase + KLINST_ABC_GETA(inst);
        KlValue* b = stkbase + KLINST_ABC_GETB(inst);
        KlValue* c = stkbase + KLINST_ABC_GETC(inst);
        klexec_binop(add, a, b, c);
        break;
      }
      case KLOPCODE_SUB: {
        KlValue* a = stkbase + KLINST_ABC_GETA(inst);
        KlValue* b = stkbase + KLINST_ABC_GETB(inst);
        KlValue* c = stkbase + KLINST_ABC_GETC(inst);
        klexec_binop(sub, a, b, c);
        break;
      }
      case KLOPCODE_MUL: {
        KlValue* a = stkbase + KLINST_ABC_GETA(inst);
        KlValue* b = stkbase + KLINST_ABC_GETB(inst);
        KlValue* c = stkbase + KLINST_ABC_GETC(inst);
        klexec_binop(mul, a, b, c);
        break;
      }
      case KLOPCODE_DIV: {
        KlValue* a = stkbase + KLINST_ABC_GETA(inst);
        KlValue* b = stkbase + KLINST_ABC_GETB(inst);
        KlValue* c = stkbase + KLINST_ABC_GETC(inst);
        klexec_bindiv(div, a, b, c);
        break;
      }
      case KLOPCODE_MOD: {
        KlValue* a = stkbase + KLINST_ABC_GETA(inst);
        KlValue* b = stkbase + KLINST_ABC_GETB(inst);
        KlValue* c = stkbase + KLINST_ABC_GETC(inst);
        klexec_binmod(mod, a, b, c);
        break;
      }
      case KLOPCODE_IDIV: {
        KlValue* a = stkbase + KLINST_ABC_GETA(inst);
        KlValue* b = stkbase + KLINST_ABC_GETB(inst);
        KlValue* c = stkbase + KLINST_ABC_GETC(inst);
        klexec_binidiv(idiv, a, b, c);
        break;
      }
      case KLOPCODE_CONCAT: {
        KlValue* a = stkbase + KLINST_ABC_GETA(inst);
        KlValue* b = stkbase + KLINST_ABC_GETB(inst);
        KlValue* c = stkbase + KLINST_ABC_GETC(inst);
        klexec_savestate(callinfo->top);
        if (kl_likely(klvalue_checktype((b), KL_STRING) && klvalue_checktype((c), KL_STRING))) {
          KlString* res = klstrpool_string_concat(state->strpool, klvalue_getobj(b, KlString*), klvalue_getobj(c, KlString*));
          if (kl_unlikely(!res))
            return klstate_throw(state, KL_E_OOM, "out of memory when do concat");
          klvalue_setobj(a, res, KL_STRING);
        } else {
          KlString* op = state->common->string.concat;
          KlException exception = klexec_dobinopmethod(state, a, b, c, op);
          if (kl_likely(callinfo != state->callinfo)) { /* is a klang call */
            KlValue* newbase = callinfo->top;
            klexec_updateglobal(newbase);
          } else {
            if (kl_unlikely(exception)) return exception;
            /* stack may have grown. restore stkbase. */
            stkbase = callinfo->base;
          }
        }
        break;
      }
      case KLOPCODE_ADDI: {
        KlValue* a = stkbase + KLINST_ABI_GETA(inst);
        KlValue* b = stkbase + KLINST_ABI_GETB(inst);
        KlInt c = KLINST_ABI_GETI(inst);
        klexec_binop_i(add, a, b, c);
        break;
      }
      case KLOPCODE_SUBI: {
        KlValue* a = stkbase + KLINST_ABI_GETA(inst);
        KlValue* b = stkbase + KLINST_ABI_GETB(inst);
        KlInt c = KLINST_ABI_GETI(inst);
        klexec_binop_i(sub, a, b, c);
        break;
      }
      case KLOPCODE_MULI: {
        KlValue* a = stkbase + KLINST_ABI_GETA(inst);
        KlValue* b = stkbase + KLINST_ABI_GETB(inst);
        KlInt c = KLINST_ABI_GETI(inst);
        klexec_binop_i(mul, a, b, c);
        break;
      }
      case KLOPCODE_DIVI: {
        KlValue* a = stkbase + KLINST_ABI_GETA(inst);
        KlValue* b = stkbase + KLINST_ABI_GETB(inst);
        KlInt imm = KLINST_ABI_GETI(inst);
        klexec_bindiv_i(div, a, b, imm);
        break;
      }
      case KLOPCODE_MODI: {
        KlValue* a = stkbase + KLINST_ABI_GETA(inst);
        KlValue* b = stkbase + KLINST_ABI_GETB(inst);
        KlInt c = KLINST_ABI_GETI(inst);
        klexec_binmod_i(mod, a, b, c);
        break;
      }
      case KLOPCODE_IDIVI: {
        KlValue* a = stkbase + KLINST_ABI_GETA(inst);
        KlValue* b = stkbase + KLINST_ABI_GETB(inst);
        KlInt imm = KLINST_ABI_GETI(inst);
        klexec_binidiv_i(idiv, a, b, imm);
        break;
      }
      case KLOPCODE_ADDC: {
        KlValue* a = stkbase + KLINST_ABX_GETA(inst);
        KlValue* b = stkbase + KLINST_ABX_GETB(inst);
        KlValue* c = constants + KLINST_ABX_GETX(inst);
        klexec_binop(add, a, b, c);
        break;
      }
      case KLOPCODE_SUBC: {
        KlValue* a = stkbase + KLINST_ABX_GETA(inst);
        KlValue* b = stkbase + KLINST_ABX_GETB(inst);
        KlValue* c = constants + KLINST_ABX_GETX(inst);
        klexec_binop(sub, a, b, c);
        break;
      }
      case KLOPCODE_MULC: {
        KlValue* a = stkbase + KLINST_ABX_GETA(inst);
        KlValue* b = stkbase + KLINST_ABX_GETB(inst);
        KlValue* c = constants + KLINST_ABX_GETX(inst);
        klexec_binop(mul, a, b, c);
        break;
      }
      case KLOPCODE_DIVC: {
        KlValue* a = stkbase + KLINST_ABX_GETA(inst);
        KlValue* b = stkbase + KLINST_ABX_GETB(inst);
        KlValue* c = constants + KLINST_ABX_GETX(inst);
        klexec_bindiv(div, a, b, c);
        break;
      }
      case KLOPCODE_MODC: {
        KlValue* a = stkbase + KLINST_ABX_GETA(inst);
        KlValue* b = stkbase + KLINST_ABX_GETB(inst);
        KlValue* c = constants + KLINST_ABX_GETX(inst);
        klexec_binmod(mod, a, b, c);
        break;
      }
      case KLOPCODE_IDIVC: {
        KlValue* a = stkbase + KLINST_ABC_GETA(inst);
        KlValue* b = stkbase + KLINST_ABC_GETB(inst);
        KlValue* c = constants + KLINST_ABC_GETC(inst);
        klexec_binidiv(idiv, a, b, c);
        break;
      }
      case KLOPCODE_NEG: {
        KlValue* b = stkbase + KLINST_ABC_GETB(inst);
        if (kl_likely(klvalue_checktype(b, KL_INT))) {
          klvalue_setint(stkbase + KLINST_ABC_GETA(inst), -klvalue_getint(b));
        } else {
          klexec_savestate(callinfo->top);
          KlException exception = klexec_dopreopmethod(state, stkbase + KLINST_ABC_GETA(inst), b, state->common->string.neg);
          if (kl_likely(callinfo != state->callinfo)) { /* is a klang call */
            KlValue* newbase = callinfo->top;
            klexec_updateglobal(newbase);
          } else {
            if (kl_unlikely(exception)) return exception;
            /* stack may have grown. restore stkbase. */
            stkbase = callinfo->base;
          }
        }
        break;
      }
      case KLOPCODE_SCALL: {
        KlValue* callable = stkbase + KLINST_AXY_GETA(inst);
        size_t narg = KLINST_AXY_GETX(inst);
        if (narg == KLINST_VARRES)
          narg = klstate_stktop(state) - callable - 1;
        size_t nret = KLINST_AXY_GETY(inst);
        klexec_savestate(callable + 1 + narg);
        KlCallInfo* newci = klexec_new_callinfo(state, nret, -1);
        if (kl_unlikely(!newci))
          return klstate_throw(state, KL_E_OOM, "out of memory when calling a callable object");
        KlException exception = klexec_callprepare(state, callable, narg, klexec_callprep_callback_for_call);
        if (kl_likely(callinfo != state->callinfo)) { /* is a klang call ? */
          KlValue* newbase = state->callinfo->base;
          klexec_updateglobal(newbase);
        } else {
          if (kl_unlikely(exception)) return exception;
          /* C function or C closure */
          /* stack may have grown. restore stkbase. */
          stkbase = callinfo->base;
        }
        break;
      }
      case KLOPCODE_CALL: {
        KlValue* callable = stkbase + KLINST_AXY_GETA(inst);
        KlInstruction extra = *pc++;
        kl_assert(KLINST_GET_OPCODE(extra) == KLOPCODE_EXTRA, "something wrong in code generation");
        size_t narg = KLINST_XYZ_GETX(extra);
        if (narg == KLINST_VARRES)
          narg = klstate_stktop(state) - callable - 1;
        size_t nret = KLINST_XYZ_GETY(extra);
        klexec_savestate(callable + 1 + narg);
        KlCallInfo* newci = klexec_new_callinfo(state, nret, (stkbase + KLINST_AXY_GETY(extra)) - callable);
        if (kl_unlikely(!newci))
          return klstate_throw(state, KL_E_OOM, "out of memory when calling a callable object");
        KlException exception = klexec_callprepare(state, callable, narg, klexec_callprep_callback_for_call);
        if (kl_likely(callinfo != state->callinfo)) { /* is a klang call ? */
          KlValue* newbase = state->callinfo->base;
          klexec_updateglobal(newbase);
        } else {
          if (kl_unlikely(exception)) return exception;
          /* C function or C closure */
          /* stack may have grown. restore stkbase. */
          stkbase = callinfo->base;
        }
        break;
      }
      case KLOPCODE_METHOD: {
        kl_assert(klvalue_checktype(constants + KLINST_AX_GETX(inst), KL_STRING), "field name should be a string");

        KlValue* thisobj = stkbase + KLINST_AX_GETA(inst);
        KlInstruction extra = *pc++;
        kl_assert(KLINST_GET_OPCODE(extra) == KLOPCODE_EXTRA, "something wrong in code generation");
        bool ismethod = klexec_is_method(thisobj);
        size_t narg = KLINST_XYZ_GETX(extra);
        if (narg == KLINST_VARRES)
          narg = klstate_stktop(state) - thisobj - 1;
        if (ismethod) ++narg;
        size_t nret = KLINST_XYZ_GETY(extra);

        klexec_savestate(thisobj + 1 + narg);
        kl_assert(KLINST_AX_GETX(extra) != KLINST_VARRES || (stkbase + KLINST_XYZ_GETZ(extra)) == thisobj, "");
        KlCallInfo* newci = klexec_new_callinfo(state, nret, (stkbase + KLINST_XYZ_GETZ(extra)) - (ismethod ? thisobj : thisobj + 1));
        if (kl_unlikely(!newci))
          return klstate_throw(state, KL_E_OOM, "out of memory when calling a callable object");
        KlString* field = klvalue_getobj(constants + KLINST_AX_GETX(inst), KlString*);
        KlValue* callable = klexec_getfield(state, thisobj, field);
        KlException exception = klexec_callprepare(state, callable, narg, klexec_callprep_callback_for_method);
        if (kl_likely(callinfo != state->callinfo)) { /* is a klang call ? */
          KlValue* newbase = state->callinfo->base;
          klexec_updateglobal(newbase);
        } else {
          if (kl_unlikely(exception)) return exception;
          /* C function or C closure */
          /* stack may have grown. restore stkbase. */
          stkbase = callinfo->base;
        }
        break;
      }
      case KLOPCODE_RETURN: {
        size_t nret = callinfo->nret;
        KlValue* res = stkbase + KLINST_AX_GETA(inst);
        size_t nres = KLINST_AX_GETX(inst);
        if (nres == KLINST_VARRES)
          nres = klstate_stktop(state) - res;
        kl_assert(KLINST_VARRES == 255, "");
        size_t ncopy = nres < nret ? nres : nret;
        KlValue* retpos = stkbase + callinfo->retoff;
        while (ncopy--) /* copy results to their position. */
          klvalue_setvalue(retpos++, res++);
        if (nret == KLINST_VARRES) {
          klstack_set_top(klstate_stack(state), retpos);
        } else if (nres < nret) { /* complete missing returned value */
          klexec_setnils(retpos, nret - nres);
        }
        klexec_pop_callinfo(state);
        if (kl_unlikely(callinfo->status & KLSTATE_CI_STATUS_STOP))
          return KL_E_NONE;
        KlValue* newbase = state->callinfo->base;
        klexec_updateglobal(newbase);
        break;
      }
      case KLOPCODE_RETURN0: {
        size_t nret = callinfo->nret;
        if (nret == KLINST_VARRES) {
          klstack_set_top(klstate_stack(state), stkbase + callinfo->retoff);
        } else {
          klexec_setnils(stkbase + callinfo->retoff, nret);
        }
        klexec_pop_callinfo(state);
        if (kl_unlikely(callinfo->status & KLSTATE_CI_STATUS_STOP))
          return KL_E_NONE;
        KlValue* newbase = state->callinfo->base;
        klexec_updateglobal(newbase);
        break;
      }
      case KLOPCODE_RETURN1: {
        kl_assert(KLINST_VARRES != 0, "");
        KlValue* retpos = stkbase + callinfo->retoff;
        KlValue* res = stkbase + KLINST_A_GETA(inst);
        size_t nret = callinfo->nret;
        if (kl_likely(nret == 1)) {
          klvalue_setvalue(retpos, res);
        } else if (nret == KLINST_VARRES) {
          klvalue_setvalue(retpos, res);
          klstack_set_top(klstate_stack(state), retpos + 1);
        } else if (nret != 0) {
          klvalue_setvalue(retpos, res);
          klexec_setnils(retpos + 1, nret - 1);   /* complete missing results */
        }
        klexec_pop_callinfo(state);
        if (kl_unlikely(callinfo->status & KLSTATE_CI_STATUS_STOP))
          return KL_E_NONE;
        KlValue* newbase = state->callinfo->base;
        klexec_updateglobal(newbase);
        break;
      }
      case KLOPCODE_LOADBOOL: {
        KlValue* a = stkbase + KLINST_AX_GETA(inst);
        KlBool boolval = KLINST_AX_GETX(inst);
        kl_assert(boolval == KL_TRUE || boolval == KL_FALSE, "instruction format error: LOADBOOL");
        klvalue_setbool(a, boolval);
        break;
      }
      case KLOPCODE_LOADI: {
        KlValue* a = stkbase + KLINST_AI_GETA(inst);
        KlInt intval = KLINST_AI_GETI(inst);
        klvalue_setint(a, intval);
        break;
      }
      case KLOPCODE_LOADC: {
        KlValue* a = stkbase + KLINST_AX_GETA(inst);
        KlValue* c = constants + KLINST_AX_GETX(inst);
        klvalue_setvalue(a, c);
        break;
      }
      case KLOPCODE_LOADNIL: {
        KlValue* a = stkbase + KLINST_AX_GETA(inst);
        size_t count = KLINST_AX_GETX(inst);
        klexec_setnils(a, count);
        break;
      }
      case KLOPCODE_LOADREF: {
        KlValue* a = stkbase + KLINST_AX_GETA(inst);
        KlValue* ref = klref_getval(closure->refs[KLINST_AX_GETX(inst)]);
        klvalue_setvalue(a, ref);
        break;
      }
      case KLOPCODE_LOADGLOBAL: {
        KlValue* a = stkbase + KLINST_AX_GETA(inst);
        kl_assert(klvalue_checktype(constants + KLINST_AX_GETX(inst), KL_STRING), "something wrong in code generation");
        KlString* varname = klvalue_getobj(constants + KLINST_AX_GETX(inst), KlString*);
        KlMapIter itr = klmap_searchstring(state->global, varname);
        itr ? klvalue_setvalue(a, &itr->value) : klvalue_setnil(a);
        break;
      }
      case KLOPCODE_STOREREF: {
        KlValue* a = stkbase + KLINST_AX_GETA(inst);
        KlValue* ref = klref_getval(closure->refs[KLINST_AX_GETX(inst)]);
        klvalue_setvalue(ref, a);
        break;
      }
      case KLOPCODE_STOREGLOBAL: {
        KlValue* a = stkbase + KLINST_AX_GETA(inst);
        kl_assert(klvalue_checktype(constants + KLINST_AX_GETX(inst), KL_STRING), "something wrong in code generation");
        KlString* varname = klvalue_getobj(constants + KLINST_AX_GETX(inst), KlString*);
        KlMapIter itr = klmap_searchstring(state->global, varname);
        if (kl_likely(itr)) {
          klvalue_setvalue(&itr->value, a);
        } else {
          klexec_savestate(callinfo->top);
          if (kl_unlikely(!klmap_insertstring(state->global, varname, a)))
            return klstate_throw(state, KL_E_OOM, "out of memory when setting a global variable");
        }
        break;
      }
      case KLOPCODE_MKMAP: {
        KlValue* a = stkbase + KLINST_ABX_GETA(inst);
        /* this instruction tells us current stack top */
        KlValue* stktop = stkbase + KLINST_ABX_GETB(inst);
        size_t capacity = KLINST_ABX_GETX(inst);
        klexec_savestate(stktop); /* creating map may trigger gc */
        KlMap* map = klmap_create(state->common->klclass.map, capacity, state->mapnodepool);
        if (kl_unlikely(!map))
          return klstate_throw(state, KL_E_OOM, "out of memory when creating a map");
        klvalue_setobj(a, map, KL_MAP);
        break;
      }
      case KLOPCODE_MKARRAY: {
        KlValue* a = stkbase + KLINST_ABX_GETA(inst);
        /* first value to be inserted to the array */
        KlValue* first = stkbase + KLINST_ABX_GETB(inst);
        size_t nelem = KLINST_ABX_GETX(inst);
        if (nelem == KLINST_VARRES)
          nelem = klstate_stktop(state) - first;
        /* now stack top is first + nelem */
        klexec_savestate(first + nelem);  /* creating array may trigger gc */
        KlArray* arr = klarray_create(state->common->klclass.array, klstate_getmm(state), nelem);
        if (kl_unlikely(!arr))
          return klstate_throw(state, KL_E_OOM, "out of memory when creating an array");
        KlArrayIter iter = klarray_iter_begin(arr);
        while(nelem--) {  /* fill this array with values on stack */
          klvalue_setvalue(iter, first++);
          iter = klarray_iter_next(iter);
        }
        klvalue_setobj(a, arr, KL_ARRAY);
        break;
      }
      case KLOPCODE_MKMETHOD:
      case KLOPCODE_MKCLOSURE: {
        kl_assert(KLINST_AX_GETX(inst) < closure->kfunc->nsubfunc, "");
        KlKFunction* kfunc = klkfunc_subfunc(closure->kfunc)[KLINST_AX_GETX(inst)];
        klexec_savestate(callinfo->top);
        KlKClosure* kclo = klkclosure_create(klstate_getmm(state), kfunc, stkbase, &state->reflist, closure->refs);
        if (kl_unlikely(!kclo))
          return klstate_throw(state, KL_E_OOM, "out of memory when creating a closure");
        if (opcode == KLOPCODE_MKMETHOD)
          klclosure_set(kclo, KLCLO_STATUS_METH);
        KlValue* a = stkbase + KLINST_AX_GETA(inst);
        klvalue_setobj(a, kclo, KL_KCLOSURE);
        break;
      }
      case KLOPCODE_APPEND: {
        KlValue* a = stkbase + KLINST_ABX_GETA(inst);
        /* first value to be inserted to the array */
        KlValue* first = stkbase + KLINST_ABX_GETB(inst);
        size_t nelem = KLINST_ABX_GETX(inst);
        if (nelem == KLINST_VARRES)
          nelem = klstate_stktop(state) - first;
        /* now stack top is first + nelem */
        klexec_savestate(first + nelem);  /* creating array may trigger gc */
        if (kl_likely(klvalue_checktype(a, KL_ARRAY))) {
          klarray_push_back(klvalue_getobj(a, KlArray*), klstate_getmm(state), first, nelem);
        } else {
          KlException exception = klexec_domultiargsmethod(state, a, a, nelem, state->common->string.append);
          if (kl_likely(callinfo != state->callinfo)) { /* is a klang call ? */
            KlValue* newbase = state->callinfo->base;
            klexec_updateglobal(newbase);
          } else {
            if (kl_unlikely(exception)) return exception;
            /* C function or C closure */
            /* stack may have grown. restore stkbase. */
            stkbase = callinfo->base;
          }
        }
        break;
      }
      case KLOPCODE_MKCLASS: {
        KlValue* a = stkbase + KLINST_ABTX_GETA(inst);
        /* this instruction tells us current stack top */
        KlValue* stktop = stkbase + KLINST_ABTX_GETB(inst);
        size_t capacity = KLINST_ABTX_GETX(inst);
        KlClass* klclass = NULL;
        if (KLINST_ABTX_GETT(inst)) { /* is stktop base class ? */
          if (kl_unlikely(!klvalue_checktype(stktop, KL_CLASS))) {
            return klstate_throw(state, KL_E_OOM, "inherit a non-class value, type: %s", klvalue_typename(klvalue_gettype(stktop)));
          }
          klexec_savestate(stktop + 1);   /* creating class may trigger gc */
          klclass = klclass_inherit(klstate_getmm(state), klvalue_getobj(stktop, KlClass*));
        } else {  /* this class has no base */
          klexec_savestate(stktop); /* creating class may trigger gc */
          klclass = klclass_create(klstate_getmm(state), capacity, KLOBJECT_DEFAULT_ATTROFF, NULL, NULL);
        }
        if (kl_unlikely(!klclass))
          return klstate_throw(state, KL_E_OOM, "out of memory when creating a class");
        klvalue_setobj(a, klclass, KL_CLASS);
        break;
      }
      case KLOPCODE_INDEXI: {
        KlValue* val = stkbase + KLINST_ABI_GETA(inst);
        KlValue* indexable = stkbase + KLINST_ABI_GETB(inst);
        KlInt index = KLINST_ABI_GETI(inst);
        if (klvalue_checktype(indexable, KL_ARRAY)) {       /* is array? */
          KlArray* arr = klvalue_getobj(indexable, KlArray*);
          klarray_index(arr, index, val);
        } else {
          KlValue key;
          klvalue_setint(&key, index);
          if (klvalue_checktype(indexable, KL_MAP)) {  /* is map? */
            KlMap* map = klvalue_getobj(indexable, KlMap*);
            KlMapIter itr = klmap_search(map, &key);
            itr ? klvalue_setvalue(val, &itr->value) : klvalue_setnil(val);
          } 
          klexec_savestate(callinfo->top);
          KlException exception = klexec_doindexmethod(state, val, indexable, &key);
          if (kl_likely(callinfo != state->callinfo)) { /* is a klang call ? */
            KlValue* newbase = state->callinfo->base;
            klexec_updateglobal(newbase);
          } else {
            if (kl_unlikely(exception)) return exception;
            /* C function or C closure */
            /* stack may have grown. restore stkbase. */
            stkbase = callinfo->base;
          }
        }
        break;
      }
      case KLOPCODE_INDEXASI: {
        KlValue* val = stkbase + KLINST_ABI_GETA(inst);
        KlValue* indexable = stkbase + KLINST_ABI_GETB(inst);
        KlInt index = KLINST_ABI_GETI(inst);
        if (klvalue_checktype(indexable, KL_ARRAY)) {         /* is array? */
          KlArray* arr = klvalue_getobj(indexable, KlArray*);
          KlException exception = klarray_indexas(arr, index, val);
          if (kl_unlikely(exception)) {
            KlValue key;
            klvalue_setint(&key, index);
            klexec_savestate(callinfo->top);
            return klexec_handle_arrayindexas_exception(state, exception, arr, &key);
          }
        } else {
          KlValue key;
          klvalue_setint(&key, index);
          if (klvalue_checktype(indexable, KL_MAP)) {  /* is map? */
            KlMap* map = klvalue_getobj(indexable, KlMap*);
            KlMapIter itr = klmap_search(map, &key);
            if (itr) {
              klvalue_setvalue(&itr->value, val);
            } else {
              klexec_savestate(callinfo->top);
              if (kl_unlikely(!klmap_insert(map, &key, val)))
                return klstate_throw(state, KL_E_OOM, "out of memory when inserting a k-v pair to a map");
            }
            break;
          }
          klexec_savestate(callinfo->top);
          KlException exception = klexec_doindexasmethod(state, val, indexable, &key);
          if (kl_likely(callinfo != state->callinfo)) { /* is a klang call ? */
            KlValue* newbase = state->callinfo->base;
            klexec_updateglobal(newbase);
          } else {
            if (kl_unlikely(exception)) return exception;
            /* C function or C closure */
            /* stack may have grown. restore stkbase. */
            stkbase = callinfo->base;
          }
        }
        break;
      }
      case KLOPCODE_INDEX: {
        KlValue* val = stkbase + KLINST_ABC_GETA(inst);
        KlValue* indexable = stkbase + KLINST_ABC_GETB(inst);
        KlValue* key = stkbase + KLINST_ABC_GETC(inst);
        if (klvalue_checktype(indexable, KL_ARRAY)) {       /* is array? */
          KlArray* arr = klvalue_getobj(indexable, KlArray*);
          if (kl_unlikely(!klvalue_checktype(key, KL_INT))) { /* only integer can index array */
            klexec_savestate(callinfo->top);
            return klstate_throw(state, KL_E_TYPE,
                                 "type error occurred when indexing an array: expected %s, got %s.",
                                 klvalue_typename(KL_INT), klvalue_typename(klvalue_gettype(key)));
          }
          klarray_index(arr, klvalue_getint(key), val);
        } else {
          if (klvalue_checktype(indexable, KL_MAP)) {  /* is map? */
            KlMap* map = klvalue_getobj(indexable, KlMap*);
            if (klvalue_canrawequal(key)) {
              KlMapIter itr = klmap_search(map, key);
              itr ? klvalue_setvalue(val, &itr->value) : klvalue_setnil(val);
              break;
            }
          }
          klexec_savestate(callinfo->top);
          KlException exception = klexec_doindexmethod(state, val, indexable, key);
          if (kl_likely(callinfo != state->callinfo)) { /* is a klang call ? */
            KlValue* newbase = state->callinfo->base;
            klexec_updateglobal(newbase);
          } else {
            if (kl_unlikely(exception)) return exception;
            /* C function or C closure */
            /* stack may have grown. restore stkbase. */
            stkbase = callinfo->base;
          }
        }
        break;
      }
      case KLOPCODE_INDEXAS: {
        KlValue* val = stkbase + KLINST_ABC_GETA(inst);
        KlValue* indexable = stkbase + KLINST_ABC_GETB(inst);
        KlValue* key = stkbase + KLINST_ABC_GETC(inst);
        if (klvalue_checktype(indexable, KL_ARRAY)) {         /* is array? */
          KlArray* arr = klvalue_getobj(indexable, KlArray*);
          klexec_savestate(callinfo->top);
          if (kl_unlikely(!klvalue_checktype(key, KL_INT))) { /* only integer can index array */
            return klstate_throw(state, KL_E_TYPE,
                                 "type error occurred when indexing an array: expected %s, got %s.",
                                 klvalue_typename(KL_INT), klvalue_typename(klvalue_gettype(key)));
          }
          KlException exception = klarray_indexas(arr, klvalue_getint(key), val);
          if (kl_unlikely(exception))
            return klexec_handle_arrayindexas_exception(state, exception, arr, key);
        } else {
          if (klvalue_checktype(indexable, KL_MAP)) {  /* is map? */
            KlMap* map = klvalue_getobj(indexable, KlMap*);
            if (klvalue_canrawequal(key)) { /* simple types that can apply raw equal. fast search and set */
              KlMapIter itr = klmap_search(map, key);
              if (itr) {
                klvalue_setvalue(&itr->value, val);
              } else {
                klexec_savestate(callinfo->top);
                if (kl_unlikely(!klmap_insert(map, key, val)))
                  return klstate_throw(state, KL_E_OOM, "out of memory when inserting a k-v pair to a map");
              }
              break;
            } /* else fall through. try operator method */
          }
          klexec_savestate(callinfo->top);
          KlException exception = klexec_doindexasmethod(state, val, indexable, key);
          if (kl_likely(callinfo != state->callinfo)) { /* is a klang call ? */
            KlValue* newbase = state->callinfo->base;
            klexec_updateglobal(newbase);
          } else {
            if (kl_unlikely(exception)) return exception;
            /* C function or C closure */
            /* stack may have grown. restore stkbase. */
            stkbase = callinfo->base;
          }
        }
        break;
      }
      case KLOPCODE_GETFIELDR: {
        KlValue* val = stkbase + KLINST_ABC_GETA(inst);
        KlValue* dotable = stkbase + KLINST_ABC_GETB(inst);
        KlValue* key = stkbase + KLINST_ABC_GETC(inst);
        klexec_getfieldgeneric(state, dotable, key, val);
        break;
      }
      case KLOPCODE_GETFIELDC: {
        KlValue* val = stkbase + KLINST_ABX_GETA(inst);
        KlValue* dotable = stkbase + KLINST_ABX_GETB(inst);
        KlValue* key = constants + KLINST_ABX_GETX(inst);
        klexec_getfieldgeneric(state, dotable, key, val);
        break;
      }
      case KLOPCODE_SETFIELDR: {
        KlValue* val = stkbase + KLINST_ABX_GETA(inst);
        KlValue* dotable = stkbase + KLINST_ABX_GETB(inst);
        KlValue* key = stkbase + KLINST_ABC_GETC(inst);
        KlException exception = klexec_setfieldgeneric(state, dotable, key, val);
        if (kl_unlikely(exception)) {
          klexec_savepc(callinfo, pc);
          return exception;
        }
        break;
      }
      case KLOPCODE_SETFIELDC: {
        KlValue* val = stkbase + KLINST_ABX_GETA(inst);
        KlValue* dotable = stkbase + KLINST_ABX_GETB(inst);
        KlValue* key = constants + KLINST_ABX_GETX(inst);
        KlException exception = klexec_setfieldgeneric(state, dotable, key, val);
        if (kl_unlikely(exception)) {
          klexec_savepc(callinfo, pc);
          return exception;
        }
        break;
      }
      case KLOPCODE_REFGETFIELDR: {
        KlValue* val = stkbase + KLINST_ABX_GETA(inst);
        KlValue* key = stkbase + KLINST_ABX_GETB(inst);
        KlValue* dotable = klref_getval(klkclosure_getref(closure, KLINST_ABX_GETX(inst)));
        klexec_getfieldgeneric(state, dotable, key, val);
        break;
      }
      case KLOPCODE_REFGETFIELDC: {
        KlValue* val = stkbase + KLINST_AXY_GETA(inst);
        KlValue* key = constants + KLINST_AXY_GETX(inst);
        KlValue* dotable = klref_getval(klkclosure_getref(closure, KLINST_AXY_GETY(inst)));
        klexec_getfieldgeneric(state, dotable, key, val);
        break;
      }
      case KLOPCODE_REFSETFIELDR: {
        KlValue* val = stkbase + KLINST_ABX_GETA(inst);
        KlValue* key = stkbase + KLINST_ABX_GETB(inst);
        KlValue* dotable = klref_getval(klkclosure_getref(closure, KLINST_ABX_GETX(inst)));
        KlException exception = klexec_setfieldgeneric(state, dotable, key, val);
        if (kl_unlikely(exception)) {
          klexec_savepc(callinfo, pc);
          return exception;
        }
        break;
      }
      case KLOPCODE_REFSETFIELDC: {
        KlValue* val = stkbase + KLINST_AXY_GETA(inst);
        KlValue* key = constants + KLINST_AXY_GETX(inst);
        KlValue* dotable = klref_getval(klkclosure_getref(closure, KLINST_AXY_GETY(inst)));
        KlException exception = klexec_setfieldgeneric(state, dotable, key, val);
        if (kl_unlikely(exception)) {
          klexec_savepc(callinfo, pc);
          return exception;
        }
        break;
      }
      case KLOPCODE_NEWLOCAL: {
        KlValue* classval = stkbase + KLINST_AX_GETA(inst);
        KlValue* fieldname = constants + KLINST_AX_GETX(inst);
        kl_assert(klvalue_checktype(classval, KL_CLASS), "NEWLOCAL should applied to a class");
        kl_assert(klvalue_checktype(fieldname, KL_STRING), "expected string to index field");

        KlString* keystr = klvalue_getobj(fieldname, KlString*);
        KlClass* klclass = klvalue_getobj(classval, KlClass*);
        klexec_savestate(callinfo->top);  /* add new field */
        KlException exception = klclass_newlocal(klclass, klstate_getmm(state), keystr);
        if (kl_unlikely(exception))
          return klexec_handle_newlocal_exception(state, exception, keystr);
        break;
      }
      case KLOPCODE_LOADFALSESKIP: {
        KlValue* a = stkbase + KLINST_A_GETA(inst);
        klvalue_setbool(a, KL_FALSE);
        ++pc;
        break;
      }
      case KLOPCODE_TESTSET: {
          KlValue* a = stkbase + KLINST_ABC_GETA(inst);
        KlValue* b = stkbase + KLINST_ABC_GETB(inst);
        kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
        KlInstruction extra = *pc++;
        bool cond = KLINST_XI_GETX(extra);
        if (klexec_satisfy(b, cond)) {
          klvalue_setvalue(a, b);
          pc += KLINST_XI_GETI(extra);
        }
        break;
      }
      case KLOPCODE_TRUEJMP: {
        KlValue* a = stkbase + KLINST_ABC_GETA(inst);
        int offset = KLINST_XI_GETI(inst);
        if (klexec_satisfy(a, KL_TRUE)) pc += offset;
        break;
      }
      case KLOPCODE_FALSEJMP: {
        KlValue* a = stkbase + KLINST_ABC_GETA(inst);
        int offset = KLINST_XI_GETI(inst);
        if (klexec_satisfy(a, KL_FALSE)) pc += offset;
        break;
      }
      case KLOPCODE_JMP: {
        int offset = KLINST_I_GETI(inst);
        pc += offset;
        break;
      }
      case KLOPCODE_CONDJMP: {
        /* This instruction must follow a comparison instruction.
         * This instruction can only be executed if the preceding comparison
         * instruction invoked the object's comparison method and the method
         * is not C function, and the method has returned. In this case, the
         * comparison result is stored at 'callinfo->top'.
         */
        bool cond = KLINST_XI_GETX(inst);
        KlValue* val = callinfo->top;
        if (klexec_satisfy(val, cond))
          pc += KLINST_XI_GETI(inst);
        break;
      }
      case KLOPCODE_CLOSEJMP: {
        KlValue* bound = stkbase + KLINST_XI_GETX(inst);
        klreflist_close(&state->reflist, bound, klstate_getmm(state));
        pc += KLINST_XI_GETI(inst);
        break;
      }
      case KLOPCODE_HASFIELD: {
        KlValue* a = stkbase + KLINST_AX_GETA(inst);
        KlValue* field = constants + KLINST_AX_GETX(inst);
        kl_assert(klvalue_checktype(field, KL_STRING), "");
        kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
        KlInstruction extra = *pc++;
        bool cond = KLINST_XI_GETX(extra);
        int offset = KLINST_XI_GETI(extra);
        if (!klvalue_checktype(klexec_getfield(state, a, klvalue_getobj(field, KlString*)), KL_NIL)) {
          if (cond) pc += offset;
        } else {
          if (!cond) pc += offset;
        }
        break;
      }
      case KLOPCODE_IS: {
        KlValue* a = stkbase + KLINST_ABC_GETA(inst);
        KlValue* b = stkbase + KLINST_ABC_GETB(inst);
        kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
        KlInstruction extra = *pc++;
        bool cond = KLINST_XI_GETX(extra);
        int offset = KLINST_XI_GETI(extra);
        if (klvalue_sametype(a, b) && klvalue_sameinstance(a, b)) {
          if (cond) pc += offset;
        } else {
          if (!cond) pc += offset;
        }
        break;
      }
      case KLOPCODE_EQ: {
        KlValue* a = stkbase + KLINST_ABC_GETA(inst);
        KlValue* b = stkbase + KLINST_ABC_GETB(inst);
        kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
        KlInstruction extra = *pc++;
        bool cond = KLINST_XI_GETX(extra);
        int offset = KLINST_XI_GETI(extra);
        klexec_bequal(a, b, offset, cond);
        break;
      }
      case KLOPCODE_NE: {
        KlValue* a = stkbase + KLINST_ABC_GETA(inst);
        KlValue* b = stkbase + KLINST_ABC_GETB(inst);
        kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
        KlInstruction extra = *pc++;
        bool cond = KLINST_XI_GETX(extra);
        int offset = KLINST_XI_GETI(extra);
        klexec_bnequal(a, b, offset, cond);
        break;
      }
      case KLOPCODE_LT: {
        KlValue* a = stkbase + KLINST_ABC_GETA(inst);
        KlValue* b = stkbase + KLINST_ABC_GETB(inst);
        kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
        KlInstruction condjmp = *pc++;
        bool cond = KLINST_XI_GETX(condjmp);
        int offset = KLINST_XI_GETI(condjmp);
        klexec_order(lt, a, b, offset, cond);
        break;
      }
      case KLOPCODE_GT: {
        KlValue* a = stkbase + KLINST_ABC_GETA(inst);
        KlValue* b = stkbase + KLINST_ABC_GETB(inst);
        kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
        KlInstruction condjmp = *pc++;
        bool cond = KLINST_XI_GETX(condjmp);
        int offset = KLINST_XI_GETI(condjmp);
        klexec_order(gt, a, b, offset, cond);
        break;
      }
      case KLOPCODE_LE: {
        KlValue* a = stkbase + KLINST_ABC_GETA(inst);
        KlValue* b = stkbase + KLINST_ABC_GETB(inst);
        kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
        KlInstruction condjmp = *pc++;
        bool cond = KLINST_XI_GETX(condjmp);
        int offset = KLINST_XI_GETI(condjmp);
        klexec_order(le, a, b, offset, cond);
        break;
      }
      case KLOPCODE_GE: {
        KlValue* a = stkbase + KLINST_ABC_GETA(inst);
        KlValue* b = stkbase + KLINST_ABC_GETB(inst);
        kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
        KlInstruction condjmp = *pc++;
        bool cond = KLINST_XI_GETX(condjmp);
        int offset = KLINST_XI_GETI(condjmp);
        klexec_order(ge, a, b, offset, cond);
        break;
      }
      case KLOPCODE_EQC: {
        KlValue* a = stkbase + KLINST_AX_GETA(inst);
        KlValue* b = constants + KLINST_AX_GETX(inst);
        kl_assert(klvalue_canrawequal(b), "something wrong in EQC");
        kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
        KlInstruction condjmp = *pc++;
        int offset = KLINST_XI_GETI(condjmp);
        if (klvalue_equal(a, b))
          pc += offset;
        break;
      }
      case KLOPCODE_NEC: {
        KlValue* a = stkbase + KLINST_AX_GETA(inst);
        KlValue* b = constants + KLINST_AX_GETX(inst);
        kl_assert(klvalue_canrawequal(b), "something wrong in NEC");
        kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
        KlInstruction condjmp = *pc++;
        int offset = KLINST_XI_GETI(condjmp);
        if (!klvalue_equal(a, b))
          pc += offset;
        break;
      }
      case KLOPCODE_LTC: {
        KlValue* a = stkbase + KLINST_AX_GETA(inst);
        KlValue* b = constants + KLINST_AX_GETX(inst);
        kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
        KlInstruction condjmp = *pc++;
        bool cond = KLINST_XI_GETX(condjmp);
        int offset = KLINST_XI_GETI(condjmp);
        klexec_order(lt, a, b, offset, cond);
        break;
      }
      case KLOPCODE_GTC: {
        KlValue* a = stkbase + KLINST_AX_GETA(inst);
        KlValue* b = constants + KLINST_AX_GETX(inst);
        kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
        KlInstruction condjmp = *pc++;
        bool cond = KLINST_XI_GETX(condjmp);
        int offset = KLINST_XI_GETI(condjmp);
        klexec_order(gt, a, b, offset, cond);
        break;
      }
      case KLOPCODE_LEC: {
        KlValue* a = stkbase + KLINST_AX_GETA(inst);
        KlValue* b = constants + KLINST_AX_GETX(inst);
        kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
        KlInstruction condjmp = *pc++;
        bool cond = KLINST_XI_GETX(condjmp);
        int offset = KLINST_XI_GETI(condjmp);
        klexec_order(le, a, b, offset, cond);
        break;
      }
      case KLOPCODE_GEC: {
        KlValue* a = stkbase + KLINST_AX_GETA(inst);
        KlValue* b = constants + KLINST_AX_GETX(inst);
        kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
        KlInstruction condjmp = *pc++;
        bool cond = KLINST_XI_GETX(condjmp);
        int offset = KLINST_XI_GETI(condjmp);
        klexec_order(ge, a, b, offset, cond);
        break;
      }
      case KLOPCODE_EQI: {
        KlValue* a = stkbase + KLINST_AI_GETA(inst);
        int imm = KLINST_AI_GETI(inst);
        kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
        KlInstruction condjmp = *pc++;
        int offset = KLINST_XI_GETI(condjmp);
        if (klvalue_checktype(a, KL_INT) && klvalue_getint(a) == imm)
          pc += offset;
        break;
      }
      case KLOPCODE_NEI: {
        KlValue* a = stkbase + KLINST_AI_GETA(inst);
        KlInt imm = KLINST_AI_GETI(inst);
        kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
        KlInstruction condjmp = *pc++;
        int offset = KLINST_XI_GETI(condjmp);
        if (!klvalue_checktype(a, KL_INT) || klvalue_getint(a) != imm)
          pc += offset;
        break;
      }
      case KLOPCODE_LTI: {
        KlValue* a = stkbase + KLINST_AI_GETA(inst);
        KlInt imm = KLINST_AI_GETI(inst);
        kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
        KlInstruction condjmp = *pc++;
        bool cond = KLINST_XI_GETX(condjmp);
        int offset = KLINST_XI_GETI(condjmp);
        klexec_orderi(lt, a, imm, offset, cond);
        break;
      }
      case KLOPCODE_GTI: {
        KlValue* a = stkbase + KLINST_AI_GETA(inst);
        KlInt imm = KLINST_AI_GETI(inst);
        kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
        KlInstruction condjmp = *pc++;
        bool cond = KLINST_XI_GETX(condjmp);
        int offset = KLINST_XI_GETI(condjmp);
        klexec_orderi(gt, a, imm, offset, cond);
        break;
      }
      case KLOPCODE_LEI: {
        KlValue* a = stkbase + KLINST_AI_GETA(inst);
        KlInt imm = KLINST_AI_GETI(inst);
        kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
        KlInstruction condjmp = *pc++;
        bool cond = KLINST_XI_GETX(condjmp);
        int offset = KLINST_XI_GETI(condjmp);
        klexec_orderi(le, a, imm, offset, cond);
        break;
      }
      case KLOPCODE_GEI: {
        KlValue* a = stkbase + KLINST_AI_GETA(inst);
        KlInt imm = KLINST_AI_GETI(inst);
        kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_CONDJMP, "");
        KlInstruction condjmp = *pc++;
        bool cond = KLINST_XI_GETX(condjmp);
        int offset = KLINST_XI_GETI(condjmp);
        klexec_orderi(ge, a, imm, offset, cond);
        break;
      }
      case KLOPCODE_MATCH: {
        KlValue* a = stkbase + KLINST_AI_GETA(inst);
        KlValue* b = constants + KLINST_AX_GETX(inst);
        if (kl_unlikely(!(klvalue_sametype(a, b) && klvalue_sameinstance(a, b)))) {
          klexec_savestate(callinfo->top);
          return klstate_throw(state, KL_E_DISMATCH, "pattern dismatch");
        }
        break;
      }
      case KLOPCODE_PMARR:
      case KLOPCODE_PBARR: {
        kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_EXTRA, "");
        KlValue* b = stkbase + KLINST_ABX_GETB(inst);
        if (!klvalue_checktype(b, KL_ARRAY)) {
          if (kl_unlikely(opcode == KLOPCODE_PBARR)) {  /* is pattern binding? */
            klexec_savestate(callinfo->top);
            return klstate_throw(state, KL_E_TYPE, "pattern binding: not an array");
          }
          /* else jump out */
          KlInstruction extra = *pc++;
          pc += KLINST_XI_GETI(extra);
          break;
        }
        KlInstruction extra = *pc++;
        /* else is array */
        KlValue* a = stkbase + KLINST_ABX_GETA(inst);
        KlArray* array = klvalue_getobj(b, KlArray*);
        size_t nfront = KLINST_ABX_GETX(inst);
        size_t arrsize = klarray_size(array);
        KlValue* end = a + (arrsize < nfront ? arrsize : nfront);
        KlValue* begin = a;
        for (KlValue* itr = klarray_iter_begin(array); begin != end; ++itr)
          klvalue_setvalue(begin++, itr);
        while (begin != a + nfront) klvalue_setnil(begin++); /* complete missing values */
        size_t nback = KLINST_XI_GETX(extra);
        begin += nback;
        kl_assert(begin == a + nfront + nback, "");
        end = begin - (arrsize < nback ? arrsize : nback);
        for (KlArrayIter itr = klarray_top(array); begin != end; --itr)
          klvalue_setvalue(begin--, klarray_iter_get(array, itr));
        while (begin != a + nfront) klvalue_setnil(begin--); /* complete missing values */
        break;
      }
      case KLOPCODE_PMTUP:
      case KLOPCODE_PBTUP: {
        KlValue* b = stkbase + KLINST_ABX_GETB(inst);
        size_t nwanted = KLINST_ABX_GETX(inst);
        KlArray* array = klvalue_getobj(b, KlArray*);
        if (!klvalue_checktype(b, KL_ARRAY) || klarray_size(array) != nwanted) {
          if (kl_unlikely(opcode == KLOPCODE_PBTUP)) {  /* is pattern binding? */
            klexec_savestate(callinfo->top);
            return klstate_throw(state, KL_E_TYPE, "pattern binding: not an array with %zd elements", nwanted);
          }
          /* else jump out */
          kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_EXTRA, "");
          KlInstruction extra = *pc++;
          pc += KLINST_XI_GETI(extra);
          break;
        }
        /* else is array with exactly 'nwanted' elements */
        if (opcode == KLOPCODE_PMTUP) /* is pattern binding? */
          ++pc; /* skip extra instruction */
        KlValue* a = stkbase + KLINST_ABX_GETA(inst);
        KlValue* end = klarray_iter_end(array);
        for (KlValue* itr = klarray_iter_begin(array); itr != end; ++itr)
          klvalue_setvalue(a++, itr);
        break;
      }
      case KLOPCODE_PMMAP:
      case KLOPCODE_PBMAP: {
        KlValue* b = stkbase + KLINST_ABX_GETB(inst);
        if (!klvalue_checktype(b, KL_MAP)) {
          if (kl_unlikely(opcode == KLOPCODE_PBMAP)) {  /* is pattern binding? */
            klexec_savestate(callinfo->top);
            return klstate_throw(state, KL_E_TYPE, "pattern binding: not an map");
          }
          /* else jump out, pattern matching instruction must be followed by extra information */
          kl_assert(KLINST_GET_OPCODE(*pc) == KLOPCODE_EXTRA, "");
          KlInstruction extra = *pc++;
          pc += KLINST_XI_GETI(extra);
          break;
        }
        ++pc; /* skip pmmappost */
        KlValue* a = stkbase + KLINST_ABX_GETA(inst);
        size_t nwanted = KLINST_ABX_GETX(inst);
        KlMap* map = klvalue_getobj(b, KlMap*);
        KlValue* key = b + 1;
        /* 'b' may be overwritten while do pattern binding,
         * so store 'b' to another position on stack that will not be overwitten
         * so that 'b' would not be collected by garbage collector */
        kl_assert(b + nwanted + 2 < klstack_size(klstate_stack(state)) + klstack_raw(klstate_stack(state)), "compiler error");
        klvalue_setvalue(b + nwanted + 1, b);
        for (size_t i = 0; i < nwanted; ++i) {
          if (kl_likely(klvalue_canrawequal(key + i))) {
            KlMapIter itr = klmap_search(map, key + i);
            itr ? klvalue_setvalue(a + i, &itr->value) : klvalue_setnil(a + i);
          } else {
            klvalue_setint(b + nwanted + 2, i); /* save current index for pmappost */
            klexec_savestate(callinfo->top);
            KlException exception = klexec_doindexmethod(state, a + i, b + nwanted + 1, key + i);
            if (kl_likely(callinfo != state->callinfo)) { /* is a klang call ? */
              KlValue* newbase = state->callinfo->base;
              klexec_updateglobal(newbase);
              --pc; /* do pmmappost instruction */
              break;  /* break to execute new function */
            } else {
              if (kl_unlikely(exception)) return exception;
              /* C function or C closure */
              /* stack may have grown. restore stkbase. */
              stkbase = callinfo->base;
            }
          }
        }
        break;
      }
      case KLOPCODE_PMAPPOST: {
        /* This instruction must follow map pattern binding or matching instruction.
         * This instruction can only be executed if the preceding instruction
         * call the index method of map and the method is not C function, and the
         * method has returned. This instruction will complete things not done by
         * previous instruction.
         */
        KlInstruction previnst = *(pc - 2);
        KlValue* b = stkbase + KLINST_ABX_GETB(previnst);
        KlValue* a = stkbase + KLINST_ABX_GETA(previnst);
        size_t nwanted = KLINST_ABX_GETX(previnst);
        KlMap* map = klvalue_getobj(b + nwanted + 1, KlMap*);
        KlValue* key = b + 1;
        kl_assert(klvalue_checktype(b + nwanted + 1, KL_MAP), "");
        kl_assert(klvalue_checktype(b + nwanted + 2, KL_INT), "");
        for (size_t i = klvalue_getint(b + nwanted + 2); i < nwanted; ++i) {
          if (kl_likely(klvalue_canrawequal(key + i))) {
            KlMapIter itr = klmap_search(map, key + i);
            itr ? klvalue_setvalue(a + i, &itr->value) : klvalue_setnil(a + i);
          } else {
            klvalue_setint(b + nwanted + 2, i); /* save current index */
            klexec_savestate(callinfo->top);
            KlException exception = klexec_doindexmethod(state, a + i, b + nwanted + 1, key + i);
            if (kl_likely(callinfo != state->callinfo)) { /* is a klang call ? */
              KlValue* newbase = state->callinfo->base;
              klexec_updateglobal(newbase);
              --pc;   /* this instruction should continue after the new function returned */
              break;  /* break to execute new function */
            } else {
              if (kl_unlikely(exception)) return exception;
              /* C function or C closure */
              /* stack may have grown. restore stkbase. */
              stkbase = callinfo->base;
            }
          }
        }
        break;
      }
      case KLOPCODE_PMOBJ:
      case KLOPCODE_PBOBJ: {
        KlValue* obj = stkbase + KLINST_ABX_GETB(inst);
        KlValue* a = stkbase + KLINST_ABX_GETA(inst);
        size_t nwanted = KLINST_ABX_GETX(inst);
        KlValue* field = obj + 1;
        for (size_t i = 0; i < nwanted; ++i) {
          kl_assert(klvalue_checktype(field + i, KL_STRING), "");
          KlString* fieldname = klvalue_getobj(field + i, KlString*);
          klvalue_setvalue(a + i, klexec_getfield(state, obj, fieldname));
        }
        break;
      }
      case KLOPCODE_NEWOBJ: {
        KlValue* klclass = stkbase + KLINST_ABC_GETB(inst);
        if (kl_unlikely(!klvalue_checktype(klclass, KL_CLASS)))
          return klstate_throw(state, KL_E_TYPE, "%s is not a class", klvalue_typename(klvalue_gettype(klclass)));
        klexec_savestate(callinfo->top);
        KlException exception = klclass_new_object(klvalue_getobj(klclass, KlClass*), klstate_getmm(state), stkbase + KLINST_ABC_GETA(inst));
        if (kl_unlikely(exception))
          return klexec_handle_newobject_exception(state, exception);
        break;
      }
      case KLOPCODE_ADJUSTARGS: {
        size_t narg = klstate_stktop(state) - stkbase;
        size_t nparam = klkfunc_nparam(closure->kfunc);
        kl_assert(narg >= nparam, "something wrong in callprepare");
        /* a closure having variable arguments needs 'narg'. */
        callinfo->narg = narg;
        if (narg > nparam) {
          /* it's not necessary to grow stack here.
           * callprepare ensures enough stack frame size.
           */
          KlValue* fixed = stkbase;
          KlValue* stktop = klstate_stktop(state);
          while (nparam--)   /* move fixed arguments to top */
            klvalue_setvalue(stktop++, fixed++);
          callinfo->top += narg;
          callinfo->base += narg;
          callinfo->retoff -= narg;
          stkbase += narg;
          klstack_set_top(klstate_stack(state), stktop);
        }
        break;
      }
      case KLOPCODE_VFORPREP: {
        KlValue* a = stkbase + KLINST_AI_GETA(inst);
        kl_assert(klvalue_checktype(a, KL_INT), "compiler error");
        KlInt nextra = callinfo->narg - klkfunc_nparam(closure->kfunc);
        KlInt step = klvalue_getint(a);
        kl_assert(step > 0, "compiler error");
        if (nextra != 0) {
          KlValue* varpos = a + 2;
          KlValue* argpos = stkbase - nextra;
          size_t nvalid = step > nextra ? nextra : step;
          klvalue_setint(a + 1, nextra - nvalid);  /* set index for next iteration */
          while (nvalid--)
            klvalue_setvalue(varpos++, argpos++);
          while (step-- > nextra)
            klvalue_setnil(varpos++);
        } else {
          pc += KLINST_AI_GETI(inst);
        }
        break;
      }
      case KLOPCODE_VFORLOOP: {
        KlValue* a = stkbase + KLINST_AI_GETA(inst);
        kl_assert(klvalue_checktype(a, KL_INT), "");
        KlInt step = klvalue_getint(a);
        KlInt idx = klvalue_getint(a + 1);
        if (idx != 0) {
          KlValue* varpos = a + 2;
          KlValue* argpos = stkbase - idx;
          size_t nvalid = step > idx ? idx : step;
          klvalue_setint(a + 1, idx - nvalid);
          while (nvalid--)
            klvalue_setvalue(varpos++, argpos++);
          while (step-- > idx)
            klvalue_setnil(varpos++);
          pc += KLINST_AI_GETI(inst);
        }
        break;
      }
      case KLOPCODE_IFORPREP: {
        KlValue* a = stkbase + KLINST_AI_GETA(inst);
        int offset = KLINST_AI_GETI(inst);
        klexec_savestate(a + 3);
        KlException exception = klexec_iforprep(state, a, offset);
        if (kl_unlikely(exception)) return exception;
        pc = callinfo->savedpc; /* pc may be changed by klexec_iforprep() */
        break;
      }
      case KLOPCODE_IFORLOOP: {
        KlValue* a = stkbase + KLINST_AI_GETA(inst);
        kl_assert(klvalue_checktype(a, KL_INT) && klvalue_checktype(a, KL_INT) && klvalue_checktype(a, KL_INT), "");
        KlInt i = klvalue_getint(a);
        KlInt end = klvalue_getint(a + 1);
        KlInt step = klvalue_getint(a + 2);
        i += step;
        if (i != end) {
          pc += KLINST_AI_GETI(inst);
          klvalue_setint(a, i);
        }
        break;
      }
      case KLOPCODE_GFORLOOP: {
        KlValue* a = stkbase + KLINST_AX_GETA(inst);
        size_t nret = KLINST_AX_GETX(inst);
        KlValue* argbase = a + 1;
        klexec_savestate(argbase + nret);
        KlCallInfo* newci = klexec_new_callinfo(state, nret, 0);
        if (kl_unlikely(!newci))
          return klstate_throw(state, KL_E_OOM, "out of memory when do generic for loop");
        KlException exception = klexec_callprepare(state, a, nret, NULL);
        if (callinfo != state->callinfo) { /* is a klang call ? */
          KlValue* newbase = state->callinfo->base;
          klexec_updateglobal(newbase);
        } else {
          if (kl_unlikely(exception)) return exception;
          /* C function or C closure */
          /* stack may have grown. restore stkbase. */
          stkbase = callinfo->base;
          KlInstruction jmp = *pc++;
          kl_assert(KLINST_GET_OPCODE(jmp) == KLOPCODE_TRUEJMP && KLINST_AI_GETA(jmp) == KLINST_AX_GETA(inst) + 1, "");
          KlValue* testval = stkbase + KLINST_AX_GETA(inst) + 1;
          if (kl_likely(klexec_satisfy(testval, KL_TRUE)))
            pc += KLINST_AI_GETI(jmp);
        }
        break;
      }
      case KLOPCODE_ASYNC: {
        KlValue* f = stkbase + KLINST_ABC_GETA(inst);
        klexec_savestate(callinfo->top);
        if (kl_unlikely(klvalue_checktype(f, KL_KCLOSURE))) {
          return klstate_throw(state, KL_E_TYPE,
                               "async should be applied to a klang closure, got '%s'",
                               klvalue_typename(klvalue_gettype(f)));
        }
        KlState* costate = klco_create(state, klvalue_getobj(f, KlKClosure*));
        if (kl_unlikely(!costate)) return klstate_throw(state, KL_E_OOM, "out of memory when creating a coroutine");
        KlValue* a = stkbase + KLINST_ABC_GETA(inst);
        klvalue_setobj(a, costate, KL_COROUTINE);
        break;
      }
      case KLOPCODE_YIELD: {
        if (kl_unlikely(!klco_valid(&state->coinfo)))   /* is this 'state' a valid coroutine? */
          return klstate_throw(state, KL_E_INVLD, "can not yield from outside a coroutine");
        KlValue* first = stkbase + KLINST_AXY_GETA(inst);
        size_t nres = KLINST_AXY_GETX(inst);
        if (nres == KLINST_VARRES)
          nres = klstate_stktop(state) - first;
        size_t nwanted = KLINST_AXY_GETY(inst);
        klexec_savestate(first + nres);
        klco_yield(&state->coinfo, first, nres, nwanted);
        return KL_E_NONE;
      }
      case KLOPCODE_VARARG: {
        size_t nwanted = KLINST_AXY_GETX(inst);
        size_t nvarg = callinfo->narg - klkfunc_nparam(closure->kfunc);
        if (nwanted == KLINST_VARRES)
          nwanted = nvarg;
        klexec_savestate(callinfo->top);
        if (kl_unlikely(klstate_checkframe(state, nwanted)))
          return klstate_throw(state, KL_E_OOM, "out of memory when copy '...' to stack");
        size_t ncopy = nwanted < nvarg ? nwanted : nvarg;
        KlValue* a = stkbase + KLINST_AXY_GETA(inst);
        KlValue* b = stkbase - nvarg;
        while (ncopy--)
          klvalue_setvalue(a++, b++);
        if (nwanted > nvarg)
          klexec_setnils(a, nwanted - nvarg);
        if (KLINST_AXY_GETX(inst) == KLINST_VARRES)
          klstack_set_top(klstate_stack(state), a);
        break;
      }
      default: {
        kl_assert(false, "control flow should not reach here");
        break;
      }
    }
  }


}
