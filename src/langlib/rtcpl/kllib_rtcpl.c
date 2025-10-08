#include "include/klapi.h"
#include "include/common/klconfig.h"
#include "include/code/klcode.h"
#include "include/parse/klparser.h"

static KlException codetokfunc(KlState* state, KlCode* code);
static KlException mkclosure(KlState* state, KlCode* code);
static void setconstant_nonstring(KlValue* val, KlConstant* constant);
/* compiler */
static KlException compiler(KlState* state);
/* interactive compiler(only accept a statement) */
static KlException compileri(KlState* state);
/* byte code loader */
static KlException bcloader(KlState* state);
/* evaluate expression */
static KlException evaluate(KlState* state);


KlException KLCONFIG_LIBRARY_RTCPL_ENTRYFUNCNAME(KlState* state) {
  KLAPI_PROTECT(klapi_allocstack(state, 4));
  klapi_setcfunc(state, -4, compiler);
  klapi_setcfunc(state, -3, compileri);
  klapi_setcfunc(state, -2, bcloader);
  klapi_setcfunc(state, -1, evaluate);
  return klapi_return(state, 4);
}

static KlException compile(KlState* state, KlAstStmtList* (*parse)(KlParser*, KlLex*)) {
  kl_assert(klapi_narg(state) == 4, "expected exactly 4 argmuments");
  kl_assert(klapi_checktypeb(state, 0, KL_USERDATA) &&
            klapi_checktypeb(state, 1, KL_USERDATA) &&
            klapi_checkstringb(state, 2) &&
            (klapi_checkstringb(state, 3) || klapi_checktypeb(state, 3, KL_NIL)),
            "expected Ki, Ko(use type tag: KL_USERDATA), string, string(or nil)");

  Ki* input = klcast(Ki*, klapi_getuserdatab(state, 0));
  Ko* err = klcast(Ko*, klapi_getuserdatab(state, 1));
  KlString* inputname = klapi_getstringb(state, 2);
  KlString* srcfile = klapi_checkstringb(state, 3) ? klapi_getstringb(state, 3) : NULL;


  KlError klerr;
  klerror_init(&klerr, err);
  KlStrTbl* strtbl = klstrtbl_create();
  if (kl_unlikely(!strtbl))
    return klapi_throw_internal(state, KL_E_OOM, "out of memory while initializing compiler");
  KlLex lex;
  if (kl_unlikely(!kllex_init(&lex, input, &klerr, klstring_content(inputname), strtbl))) {
    klstrtbl_delete(strtbl);
    return klapi_throw_internal(state, KL_E_OOM, "out of memory while initializing compiler");
  }
  KlParser parser;
  if (kl_unlikely(!klparser_init(&parser, strtbl, klstring_content(inputname), &klerr))) {
    kllex_destroy(&lex);
    klstrtbl_delete(strtbl);
    return klapi_throw_internal(state, KL_E_OOM, "out of memory while initializing compiler");
  }

  /* parse */
  kllex_next(&lex);
  KlAstStmtList* ast = parse(&parser, &lex);
  kllex_destroy(&lex);
  if (klerror_nerror(&klerr) != 0) {
    if (ast) klast_delete(ast);
    klstrtbl_delete(strtbl);
    /* syntax error, not an exception, return nothing */
    return klapi_return(state, 0);
  }

  KlStrDesc srcfilename;
  if (!srcfile) {
    srcfilename = (KlStrDesc) { .id = 0, .length = 0 };
  } else {
    const char* str = klstrtbl_newstring(strtbl, klstring_content(srcfile));
    if (kl_unlikely(!str)) {
      klast_delete(ast);
      klstrtbl_delete(strtbl);
      return klapi_throw_internal(state, KL_E_OOM, "out of memory while initializing code generator");
    }
    srcfilename = (KlStrDesc) { .id = klstrtbl_stringid(strtbl, str), .length = klstring_length(srcfile) };
  }
  /* genenrate code */
  KlCodeGenConfig config = {
    .inputname = klstring_content(inputname),
    .srcfile = srcfilename,
    .input = input,
    .klerr = &klerr,
    .debug = false,
    .posinfo = true,
  };
  KlCode* code = klcode_create_fromast(ast, strtbl, &config);
  klast_delete(ast);
  if (klerror_nerror(&klerr) != 0) {
    if (code)
      klcode_delete(code);
    klstrtbl_delete(strtbl);
    /* semantic error, not an exception, return nothing */
    return klapi_return(state, 0);
  }
  KlException exception = mkclosure(state, code);
  klcode_delete(code);
  klstrtbl_delete(strtbl);
  if (kl_unlikely(exception))
    return exception;
  kl_assert(klapi_framesize(state) == klapi_narg(state) + 1, "");
  return klapi_return(state, 1);
}

static KlException compiler(KlState* state) {
  return compile(state, klparser_file);
}

static KlException compileri(KlState* state) {
  return compile(state, klparser_interactive);
}

static KlException evaluate(KlState* state) {
  return compile(state, klparser_evaluate);
}

static KlException bcloader(KlState* state) {
  kl_assert(klapi_narg(state) == 2, "expected exactly 2 argmuments");
  kl_assert(klapi_checktypeb(state, 0, KL_USERDATA) && klapi_checktypeb(state, 1, KL_USERDATA),
            "expected Ki, Ko(use type tag: KL_USERDATA)");

  Ki* input = klcast(Ki*, klapi_getuserdatab(state, 0));
  Ko* err = klcast(Ko*, klapi_getuserdatab(state, 1));

  KlError klerr;
  klerror_init(&klerr, err);
  KlStrTbl* strtbl = klstrtbl_create();
  if (kl_unlikely(!strtbl))
    return klapi_throw_internal(state, KL_E_OOM, "out of memory while initializing bytecode loader");

  KlUnDumpError error;
  KlCode* code = klcode_undump(input, strtbl, &error);
  if (kl_unlikely(!code)) {
    const char* errmsg = NULL;
    switch (error) {
      case KLUNDUMP_ERROR_BAD: {
        errmsg = "bad binary file";
        break;
      }
      case KLUNDUMP_ERROR_SIZE: {
        errmsg = "type size does not match";
        break;
      }
      case KLUNDUMP_ERROR_OOM: {
        errmsg = "out of memory";
        break;
      }
      case KLUNDUMP_ERROR_MAGIC: {
        errmsg = "not a bytecode file";
        break;
      }
      case KLUNDUMP_ERROR_ENDIAN: {
        errmsg = "byte order does not match";
        break;
      }
      case KLUNDUMP_ERROR_VERSION: {
        errmsg = "version does not match";
        break;
      }
      default: {
        errmsg = "unknown error";
        break;
      }
    }
    klstrtbl_delete(strtbl);
    ko_printf(err, "undump failure: %s\n", errmsg);
    return klapi_return(state, 0);
  }
  KlException exception = mkclosure(state, code);
  klcode_delete(code);
  klstrtbl_delete(strtbl);
  if (kl_unlikely(exception)) return exception;
  kl_assert(klapi_framesize(state) == klapi_narg(state) + 1, "");
  return klapi_return(state, 1);
}

static KlException mkclosure(KlState* state, KlCode* code) {
  KlException exception = codetokfunc(state, code);
  if (kl_unlikely(exception)) return exception;
  KlKClosure* kclo = klkclosure_create(klstate_getmm(state), klapi_getobj(state, -1, KlKFunction*), NULL, klstate_reflist(state), NULL);
  if (kl_unlikely(!kclo))
    return klapi_throw_internal(state, KL_E_OOM, "out of memory while creating klang closure");
  klapi_setobj(state, -1, kclo, KL_KCLOSURE);
  return KL_E_NONE;
}

static KlException codetokfunc(KlState* state, KlCode* code) {
  size_t nnested = code->nnested;
  KlCode** nestedfunc = code->nestedfunc;
  for (size_t i = 0; i < nnested; ++i) {
    KLAPI_PROTECT(codetokfunc(state, nestedfunc[i]));
  }

  KlMM* klmm = klstate_getmm(state);
  KlKFunction* kfunc = klapi_kfunc_alloc(state, code->codelen, code->nconst, code->nref, code->nnested, code->framesize, code->nparam);
  if (kl_unlikely(!kfunc)) return klapi_currexception(state);
  memcpy(klapi_kfunc_insts(state, kfunc), code->code, code->codelen * sizeof (KlInstruction));

  size_t nstkused = 0;  /* stack used from now */
  /* init source file info and posinfo */
  if (code->srcfile.length != 0) {
    KLAPI_MAYFAIL(klapi_checkstack(state, 1), klapi_kfunc_initabort(state, kfunc));
    KLAPI_MAYFAIL(klapi_pushstring_buf(state, klstrtbl_getstring(code->strtbl, code->srcfile.id), code->srcfile.length),
                  klapi_kfunc_initabort(state, kfunc));
    ++nstkused;
    klapi_kfunc_setsrcfile(state, kfunc, klapi_getstring(state, -1));
  }
  if (code->posinfo != NULL) {
    KlKFuncFilePosition* posinfo = (KlKFuncFilePosition*)klmm_alloc(klmm, code->codelen * sizeof (KlKFuncFilePosition));
    if (kl_unlikely(!posinfo)) {
      klapi_kfunc_initabort(state, kfunc);
      return klapi_throw_internal(state, KL_E_OOM, "out of memory while creating klang function");
    }
    for (size_t i = 0; i < code->codelen; ++i) {
      posinfo[i].begin = code->posinfo[i].begin;
      posinfo[i].end = code->posinfo[i].end;
    }
    klapi_kfunc_setposinfo(state, kfunc, posinfo);
  }
  /* init constants */
  size_t nconst = code->nconst;
  KlConstant* concode = code->constants;
  KlValue* conkfunc = klapi_kfunc_constants(state, kfunc);
  for (size_t i = 0; i < nconst; ++i) {
    if (concode[i].type != KLC_STRING) { /* not a gc constant */
      setconstant_nonstring(&conkfunc[i], &concode[i]);
    } else {  /* a gc constant, push it to stack to avoid being collected */
      KLAPI_MAYFAIL(klapi_checkstack(state, 1), klapi_kfunc_initabort(state, kfunc));
      KLAPI_MAYFAIL(klapi_pushstring_buf(state, klstrtbl_getstring(code->strtbl, concode[i].string.id), concode[i].string.length),
                    klapi_kfunc_initabort(state, kfunc));
      ++nstkused;
      klvalue_setvalue(&conkfunc[i], klapi_access(state, -1));
    }
  }
  /* init refinfo */
  size_t nref = code->nref;
  KlCRefInfo* refcode = code->refinfo;
  KlRefInfo* refkfunc = klapi_kfunc_refinfo(state, kfunc);
  for (size_t i = 0; i < nref; ++i) {
    refkfunc[i].index = refcode[i].index;
    refkfunc[i].on_stack = refcode[i].on_stack;
  }
  if (nnested + nstkused == 0) {
    KLAPI_MAYFAIL(klapi_allocstack(state, 1), klapi_kfunc_initabort(state, kfunc));
  } else {
    KlValue* subfuncs_onstk = klapi_access(state, -(nstkused + code->nnested));
    KlKFunction** subfuncs = klkfunc_subfunc(kfunc);
    for (size_t i = 0; i < nnested; ++i) {
      kl_assert(klvalue_checktype(&subfuncs_onstk[i], KL_KFUNCTION), "");
      subfuncs[i] = klvalue_getobj(&subfuncs_onstk[i], KlKFunction*);
    }
    klapi_pop(state, (nstkused + nnested) - 1);
  }
  klapi_setobj(state, -1, kfunc, KL_KFUNCTION);
  klkfunc_initdone(klmm, kfunc);
  return KL_E_NONE;
}

static void setconstant_nonstring(KlValue* val, KlConstant* constant) {
  switch (constant->type) {
    case KLC_INT: {
      klvalue_setint(val, constant->intval);
      return;
    }
    case KLC_BOOL: {
      klvalue_setbool(val, constant->boolval == KLC_TRUE ? KL_TRUE : KL_FALSE);
      return;
    }
    case KLC_NIL: {
      klvalue_setnil(val);
      return;
    }
    case KLC_FLOAT: {
      klvalue_setfloat(val, constant->floatval);
      return;
    }
    default: {
      kl_assert(false, "unreachable");
      return;
    }
  }
}
