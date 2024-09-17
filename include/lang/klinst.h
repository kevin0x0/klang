#ifndef _KLANG_INCLUDE_LANG_KLINST_H_
#define _KLANG_INCLUDE_LANG_KLINST_H_

#include "include/misc/klutils.h"
#include <stdint.h>


typedef uint32_t KlInstruction;
typedef uint8_t KlOpcode;


/* A, B, C, ...         represent register index
 * X, Y, Z, U, V, ...   represent unsigned immediate number
 * I, J, K, ...         represent signed immediate number
 * T                    represent tag(can be tested)
 */

#define klinst_inrange(num, width)      (klinst_utos(0, width) <= num && num <= klinst_utos(klbit(width) - 1, width))
#define klinst_inurange(num, width)     ((uintmax_t)num <= klbit(width - 1))
#define klinst_utos(num, width)         ((int32_t)(num) - (int32_t)(klbit((width) - 1)))
#define klinst_stou(num, width)         ((int32_t)(num) + (klbit((width) - 1)))

#define KLINST_GET_OPCODE(inst)         (klcast(KlOpcode, (uint8_t)(inst)))

/* instruction that has no argument */
#define klinst_(opcode)                 kllow8bits(opcode)

#define klinst_ABC(opcode, a, b, c)     ((kllow8bits(opcode)) | (kllow8bits(a) << 8) | (kllow8bits(b) << 16) | (kllow8bits(c) << 24))
#define KLINST_ABC_GETA(inst)           ((uint8_t)((inst) >> 8))
#define KLINST_ABC_GETB(inst)           ((uint8_t)((inst) >> 16))
#define KLINST_ABC_GETC(inst)           ((uint8_t)((inst) >> 24))

#define klinst_ABI(opcode, a, b, i)     ((kllow8bits(opcode)) | (kllow8bits(a) << 8) | (kllow8bits(b) << 16) | (klinst_stou((i), 8) << 24))
#define KLINST_ABI_GETA(inst)           KLINST_ABC_GETA(inst)
#define KLINST_ABI_GETB(inst)           KLINST_ABC_GETB(inst)
#define KLINST_ABI_GETI(inst)           (klinst_utos((uint8_t)((inst) >> 24), 8))

#define klinst_AXI(opcode, a, x, i)     ((kllow8bits(opcode)) | (kllow8bits(a) << 8) | (kllow8bits(x) << 16) | (klinst_stou((i), 8) << 24))
#define KLINST_AXI_GETA(inst)           KLINST_ABC_GETA(inst)
#define KLINST_AXI_GETX(inst)           KLINST_ABC_GETB(inst)
#define KLINST_AXI_GETI(inst)           (klinst_utos((uint8_t)((inst) >> 24), 8))

#define klinst_AIJ(opcode, a, i, j)     ((kllow8bits(opcode)) | (kllow8bits(a) << 8) | ((klinst_stou((i), 8) << 16)) | (klinst_stou((j), 8) << 24))
#define KLINST_AIJ_GETA(inst)           KLINST_ABC_GETA(inst)
#define KLINST_AIJ_GETI(inst)           (klinst_utos((uint8_t)((inst) >> 16), 8))
#define KLINST_AIJ_GETJ(inst)           (klinst_utos((inst) >> 24, 8))

#define klinst_ABX(opcode, a, b, x)     klinst_ABC(opcode, a, b, x)
#define KLINST_ABX_GETA(inst)           KLINST_ABC_GETA(inst)
#define KLINST_ABX_GETB(inst)           KLINST_ABC_GETB(inst)
#define KLINST_ABX_GETX(inst)           KLINST_ABC_GETC(inst)

#define klinst_AXY(opcode, a, x, y)     klinst_ABC(opcode, a, x, y)
#define KLINST_AXY_GETA(inst)           KLINST_ABC_GETA(inst)
#define KLINST_AXY_GETX(inst)           KLINST_ABC_GETB(inst)
#define KLINST_AXY_GETY(inst)           KLINST_ABC_GETC(inst)

#define klinst_XYZ(opcode, x, y, z)     klinst_ABC(opcode, x, y, z)
#define KLINST_XYZ_GETX(inst)           KLINST_ABC_GETA(inst)
#define KLINST_XYZ_GETY(inst)           KLINST_ABC_GETB(inst)
#define KLINST_XYZ_GETZ(inst)           KLINST_ABC_GETC(inst)

#define klinst_ABTX(opcode, a, b, t, x) \
  ((kllow8bits(opcode)) | (kllow8bits(a) << 8) | ((uint8_t)(b) << 16) | (kllowbits(t, 1) << 24) | (kllowbits(x, 7) << 25))
#define KLINST_ABTX_GETA(inst)          KLINST_ABC_GETA(inst)
#define KLINST_ABTX_GETB(inst)          KLINST_ABC_GETB(inst)
#define KLINST_ABTX_GETT(inst)          ((inst) & klbit(24))
#define KLINST_ABTX_GETX(inst)          ((inst) >> 25)

#define klinst_ABXY(opcode, a, b, x, y) \
  ((kllow8bits(opcode)) | (kllow8bits(a) << 8) | (kllow8bits(b) << 16) | (kllow4bits(x) << 24) | (kllow4bits(y) << 28))
#define KLINST_ABXY_GETA(inst)          KLINST_ABC_GETA(inst)
#define KLINST_ABXY_GETB(inst)          KLINST_ABC_GETB(inst)
#define KLINST_ABXY_GETX(inst)          (((inst) << 4) >> 28)
#define KLINST_ABXY_GETY(inst)          ((inst) >> 28)

#define klinst_A(opcode, a)             ((kllow8bits(opcode)) | ((uint32_t)(a) << 8))
#define KLINST_A_GETA(inst)             ((inst) >> 8)

#define klinst_X(opcode, a)             ((kllow8bits(opcode)) | ((uint32_t)(a) << 8))
#define KLINST_X_GETX(inst)             ((inst) >> 8)

#define klinst_I(opcode, a)             ((kllow8bits(opcode)) | (klinst_stou((a), 24) << 8))
#define KLINST_I_GETI(inst)             (klinst_utos((inst) >> 8, 24))

#define klinst_AX(opcode, a, x)         ((kllow8bits(opcode)) | (kllow8bits(a) << 8) | ((uint32_t)(x) << 16))
#define KLINST_AX_GETA(inst)            KLINST_ABC_GETA(inst)
#define KLINST_AX_GETX(inst)            ((inst) >> 16)

#define klinst_XY(opcode, x, y)         ((kllow8bits(opcode)) | (kllow8bits(x) << 8) | ((uint32_t)(y) << 16))
#define KLINST_XY_GETX(inst)            KLINST_ABC_GETA(inst)
#define KLINST_XY_GETY(inst)            ((inst) >> 16)

#define klinst_AI(opcode, a, i)         ((kllow8bits(opcode)) | (kllow8bits(a) << 8) | (klinst_stou((i), 16) << 16))
#define KLINST_AI_GETA(inst)            KLINST_ABC_GETA(inst)
#define KLINST_AI_GETI(inst)            klinst_utos((inst) >> 16, 16)

#define klinst_XI(opcode, x, i)         klinst_AI(opcode, x, i)
#define KLINST_XI_GETX(inst)            KLINST_AI_GETA(inst)
#define KLINST_XI_GETI(inst)            KLINST_AI_GETI(inst)




#define KLOPCODE_MOVE               (0)
#define KLOPCODE_MULTIMOVE          (1)
#define KLOPCODE_ADD                (2)
#define KLOPCODE_SUB                (3)
#define KLOPCODE_MUL                (4)
#define KLOPCODE_DIV                (5)
#define KLOPCODE_MOD                (6)
#define KLOPCODE_IDIV               (7)
#define KLOPCODE_CONCAT             (8)
#define KLOPCODE_ADDI               (9)
#define KLOPCODE_SUBI               (10)
#define KLOPCODE_MULI               (11)
#define KLOPCODE_DIVI               (12)
#define KLOPCODE_MODI               (13)
#define KLOPCODE_IDIVI              (14)
#define KLOPCODE_ADDC               (15)
#define KLOPCODE_SUBC               (16)
#define KLOPCODE_MULC               (17)
#define KLOPCODE_DIVC               (18)
#define KLOPCODE_MODC               (19)
#define KLOPCODE_IDIVC              (20)
#define KLOPCODE_NEG                (21)
#define KLOPCODE_LEN                (22)
#define KLOPCODE_SCALL              (23)
#define KLOPCODE_CALL               (24)
#define KLOPCODE_METHOD             (25)
#define KLOPCODE_RETURN             (26)
#define KLOPCODE_RETURN0            (27)
#define KLOPCODE_RETURN1            (28)
#define KLOPCODE_LOADBOOL           (29)
#define KLOPCODE_LOADI              (30)
#define KLOPCODE_LOADC              (31)
#define KLOPCODE_LOADNIL            (32)
#define KLOPCODE_LOADREF            (33)
#define KLOPCODE_LOADGLOBAL         (34)
#define KLOPCODE_STOREREF           (35)
#define KLOPCODE_STOREGLOBAL        (36)
#define KLOPCODE_MKTUPLE            (37)
#define KLOPCODE_MKMAP              (38)
#define KLOPCODE_MKARRAY            (39)
#define KLOPCODE_MKCLOSURE          (40)
#define KLOPCODE_APPEND             (41)
#define KLOPCODE_MKCLASS            (42)
#define KLOPCODE_INDEXI             (43)
#define KLOPCODE_INDEXASI           (44)
#define KLOPCODE_INDEX              (45)
#define KLOPCODE_INDEXAS            (46)
#define KLOPCODE_GETFIELDR          (47)
#define KLOPCODE_GETFIELDC          (48)
#define KLOPCODE_SETFIELDR          (49)
#define KLOPCODE_SETFIELDC          (50)
#define KLOPCODE_REFGETFIELDR       (51)
#define KLOPCODE_REFGETFIELDC       (52)
#define KLOPCODE_REFSETFIELDR       (53)
#define KLOPCODE_REFSETFIELDC       (54)
#define KLOPCODE_NEWLOCAL           (55)
#define KLOPCODE_NEWMETHODC         (56)
#define KLOPCODE_NEWMETHODR         (57)
#define KLOPCODE_LOADFALSESKIP      (58)
#define KLOPCODE_TESTSET            (59)
#define KLOPCODE_TRUEJMP            (60)
#define KLOPCODE_FALSEJMP           (61)
#define KLOPCODE_JMP                (62)
#define KLOPCODE_CONDJMP            (63)
#define KLOPCODE_CLOSEJMP           (64)
#define KLOPCODE_HASFIELD           (65)
#define KLOPCODE_IS                 (66)
#define KLOPCODE_EQ                 (67)
#define KLOPCODE_NE                 (68)
#define KLOPCODE_LE                 (69)
#define KLOPCODE_GE                 (70)
#define KLOPCODE_LT                 (71)
#define KLOPCODE_GT                 (72)
#define KLOPCODE_EQC                (73)
#define KLOPCODE_NEC                (74)
#define KLOPCODE_LEC                (75)
#define KLOPCODE_GEC                (76)
#define KLOPCODE_LTC                (77)
#define KLOPCODE_GTC                (78)
#define KLOPCODE_EQI                (79)
#define KLOPCODE_NEI                (80)
#define KLOPCODE_LEI                (81)
#define KLOPCODE_GEI                (82)
#define KLOPCODE_LTI                (83)
#define KLOPCODE_GTI                (84)
#define KLOPCODE_MATCH              (85)
#define KLOPCODE_PBARR              (86)
#define KLOPCODE_PBTUP              (87)
#define KLOPCODE_PBMAP              (88)
#define KLOPCODE_PBOBJ              (89)
#define KLOPCODE_PMARR              (90)
#define KLOPCODE_PMTUP              (91)
#define KLOPCODE_PMMAP              (92)
#define KLOPCODE_PMOBJ              (93)
#define KLOPCODE_PMAPPOST           (94)
#define KLOPCODE_NEWOBJ             (95)
#define KLOPCODE_ADJUSTARGS         (96)
#define KLOPCODE_VFORPREP           (97)
#define KLOPCODE_VFORLOOP           (98)
#define KLOPCODE_IFORPREP           (99)
#define KLOPCODE_IFORLOOP           (100)
#define KLOPCODE_GFORLOOP           (101)
#define KLOPCODE_ASYNC              (102)
#define KLOPCODE_YIELD              (103)
#define KLOPCODE_VARARG             (104)
#define KLOPCODE_NINST              (105)


/* extra information for some instructions */
#define KLOPCODE_EXTRA              (255)
  

#define KLINST_VARRES               (255)

#define klinst_move(a, b)                                 klinst_ABC(KLOPCODE_MOVE, (a), (b), 0)
#define klinst_multimove(a, b, nmove)                     klinst_ABX(KLOPCODE_MULTIMOVE, (a), (b), (nmove))
#define klinst_add(a, b, c)                               klinst_ABC(KLOPCODE_ADD, (a), (b), (c))
#define klinst_sub(a, b, c)                               klinst_ABC(KLOPCODE_SUB, (a), (b), (c))
#define klinst_mul(a, b, c)                               klinst_ABC(KLOPCODE_MUL, (a), (b), (c))
#define klinst_div(a, b, c)                               klinst_ABC(KLOPCODE_DIV, (a), (b), (c))
#define klinst_mod(a, b, c)                               klinst_ABC(KLOPCODE_MOD, (a), (b), (c))
#define klinst_idiv(a, b, c)                              klinst_ABC(KLOPCODE_IDIV, (a), (b), (c))
#define klinst_concat(a, b, c)                            klinst_ABC(KLOPCODE_CONCAT, (a), (b), (c))
#define klinst_addi(a, b, i)                              klinst_ABI(KLOPCODE_ADDI, (a), (b), (i))
#define klinst_subi(a, b, i)                              klinst_ABI(KLOPCODE_SUBI, (a), (b), (i))
#define klinst_muli(a, b, i)                              klinst_ABI(KLOPCODE_MULI, (a), (b), (i))
#define klinst_divi(a, b, i)                              klinst_ABI(KLOPCODE_DIVI, (a), (b), (i))
#define klinst_modi(a, b, i)                              klinst_ABI(KLOPCODE_MODI, (a), (b), (i))
#define klinst_idivi(a, b, i)                             klinst_ABI(KLOPCODE_IDIVI, (a), (b), (i))
#define klinst_addc(a, b, x)                              klinst_ABX(KLOPCODE_ADDC, (a), (b), (x))
#define klinst_subc(a, b, x)                              klinst_ABX(KLOPCODE_SUBC, (a), (b), (x))
#define klinst_mulc(a, b, x)                              klinst_ABX(KLOPCODE_MULC, (a), (b), (x))
#define klinst_divc(a, b, x)                              klinst_ABX(KLOPCODE_DIVC, (a), (b), (x))
#define klinst_modc(a, b, x)                              klinst_ABX(KLOPCODE_MODC, (a), (b), (x))
#define klinst_idivc(a, b, x)                             klinst_ABX(KLOPCODE_IDIVC, (a), (b), (x))
#define klinst_len(a, b)                                  klinst_ABC(KLOPCODE_LEN, (a), (b), 0)
#define klinst_neg(a, b)                                  klinst_ABC(KLOPCODE_NEG, (a), (b), 0)
#define klinst_scall(callable, narg, nret)                klinst_AXY(KLOPCODE_SCALL, (callable), (narg), (nret))
#define klinst_call(callable)                             klinst_ABC(KLOPCODE_CALL, (callable), (0), (0))
#define klinst_callextra(narg, nret, retpos)              klinst_extra_xyz(narg, nret, (retpos))
#define klinst_method(thisobj, field)                     klinst_AX(KLOPCODE_METHOD, (thisobj), (field))
#define klinst_methodextra(narg, nret, retpos)            klinst_extra_xyz(narg, nret, (retpos))
#define klinst_return(first, nres)                        klinst_AX(KLOPCODE_RETURN, (first), (nres))
#define klinst_return0()                                  klinst_(KLOPCODE_RETURN0)
#define klinst_return1(res)                               klinst_A(KLOPCODE_RETURN1, (res))
#define klinst_loadbool(a, x)                             klinst_AX(KLOPCODE_LOADBOOL, (a), (x))
#define klinst_loadi(a, i)                                klinst_AI(KLOPCODE_LOADI, (a), (i))
#define klinst_loadc(a, x)                                klinst_AX(KLOPCODE_LOADC, (a), (x))
#define klinst_loadnil(a, count)                          klinst_AX(KLOPCODE_LOADNIL, (a), (count))
#define klinst_loadref(a, refidx)                         klinst_AX(KLOPCODE_LOADREF, (a), (refidx))
#define klinst_loadglobal(a, x)                           klinst_AX(KLOPCODE_LOADGLOBAL, (a), (x))
#define klinst_storeref(a, refidx)                        klinst_AX(KLOPCODE_STOREREF, (a), (refidx))
#define klinst_storeglobal(a, x)                          klinst_AX(KLOPCODE_STOREGLOBAL, (a), (x))
#define klinst_mktuple(a, base, nval)                     klinst_ABX(KLOPCODE_MKTUPLE, (a), (base), (nval))
#define klinst_mkmap(a, stktop, capacity)                 klinst_ABX(KLOPCODE_MKMAP, (a), (stktop), (capacity))
#define klinst_mkarray(a, first, nelem)                   klinst_ABX(KLOPCODE_MKARRAY, (a), (first), (nelem))
#define klinst_mkmethod(a, x)                             klinst_AX(KLOPCODE_MKMETHOD, (a), (x))
#define klinst_mkclosure(a, x)                            klinst_AX(KLOPCODE_MKCLOSURE, (a), (x))
#define klinst_append(a, first, nelem)                    klinst_ABX(KLOPCODE_APPEND, (a), (first), (nelem))
#define klinst_mkclass(a, stktop, topisbase, capacity)    klinst_ABTX(KLOPCODE_MKCLASS, (a), (stktop), (topisbase), (capacity))
#define klinst_indexi(a, indexable, i)                    klinst_ABI(KLOPCODE_INDEXI, (a), (indexable), (i))
#define klinst_indexasi(val, indexable, i)                klinst_ABI(KLOPCODE_INDEXASI, (val), (indexable), (i))
#define klinst_index(a, indexable, key)                   klinst_ABC(KLOPCODE_INDEX, (a), (indexable), (key))
#define klinst_indexas(val, indexable, key)               klinst_ABC(KLOPCODE_INDEXAS, (val), (indexable), (key))
#define klinst_getfieldr(a, obj, field)                   klinst_ABC(KLOPCODE_GETFIELDR, (a), (obj), (field))
#define klinst_getfieldc(a, obj, field)                   klinst_ABX(KLOPCODE_GETFIELDC, (a), (obj), (field))
#define klinst_setfieldr(val, obj, field)                 klinst_ABC(KLOPCODE_SETFIELDR, (val), (obj), (field))
#define klinst_setfieldc(val, obj, field)                 klinst_ABX(KLOPCODE_SETFIELDC, (val), (obj), (field))
#define klinst_refgetfieldr(a, obj, field)                klinst_ABX(KLOPCODE_REFGETFIELDR, (a), (field), (obj))
#define klinst_refgetfieldc(a, obj, field)                klinst_AXY(KLOPCODE_REFGETFIELDC, (a), (field), (obj))
#define klinst_refsetfieldr(val, obj, field)              klinst_ABX(KLOPCODE_REFSETFIELDR, (val), (field), (obj))
#define klinst_refsetfieldc(val, obj, field)              klinst_AXY(KLOPCODE_REFSETFIELDC, (val), (field), (obj))
#define klinst_newlocal(a, field)                         klinst_AX(KLOPCODE_NEWLOCAL, (a), (field))
#define klinst_newmethodc(obj, val, field)                klinst_ABX(KLOPCODE_NEWMETHODC, (obj), (val), (field))
#define klinst_newmethodr(obj, val, field)                klinst_ABC(KLOPCODE_NEWMETHODR, (obj), (val), (field))
#define klinst_loadfalseskip(a)                           klinst_A(KLOPCODE_LOADFALSESKIP, (a))
#define klinst_testset(a, b)                              klinst_ABC(KLOPCODE_TESTSET, (a), (b), 0)
#define klinst_truejmp(a, offset)                         klinst_AI(KLOPCODE_TRUEJMP, (a), (offset))
#define klinst_falsejmp(a, offset)                        klinst_AI(KLOPCODE_FALSEJMP, (a), (offset))
#define klinst_jmp(offset)                                klinst_I(KLOPCODE_JMP, (offset))
#define klinst_condjmp(cond, offset)                      klinst_XI(KLOPCODE_CONDJMP, (cond), (offset))
#define klinst_closejmp(bound, offset)                    klinst_XI(KLOPCODE_CLOSEJMP, (bound), (offset))
#define klinst_close(bound)                               klinst_closejmp((bound), 0)
#define klinst_hasfield(a, field)                         klinst_AX(KLOPCODE_HASFIELD, (a), (field))
#define klinst_is(a, b)                                   klinst_ABC(KLOPCODE_IS, (a), (b), (0))
#define klinst_eq(a, b)                                   klinst_ABC(KLOPCODE_EQ, (a), (b), (0))
#define klinst_ne(a, b)                                   klinst_ABC(KLOPCODE_NE, (a), (b), (0))
#define klinst_le(a, b)                                   klinst_ABC(KLOPCODE_LE, (a), (b), (0))
#define klinst_ge(a, b)                                   klinst_ABC(KLOPCODE_GE, (a), (b), (0))
#define klinst_lt(a, b)                                   klinst_ABC(KLOPCODE_LT, (a), (b), (0))
#define klinst_gt(a, b)                                   klinst_ABC(KLOPCODE_GT, (a), (b), (0))
#define klinst_eqc(a, x)                                  klinst_AX(KLOPCODE_EQC, (a), (x))
#define klinst_nec(a, x)                                  klinst_AX(KLOPCODE_NEC, (a), (x))
#define klinst_lec(a, x)                                  klinst_AX(KLOPCODE_LEC, (a), (x))
#define klinst_gec(a, x)                                  klinst_AX(KLOPCODE_GEC, (a), (x))
#define klinst_ltc(a, x)                                  klinst_AX(KLOPCODE_LTC, (a), (x))
#define klinst_gtc(a, x)                                  klinst_AX(KLOPCODE_GTC, (a), (x))
#define klinst_eqi(a, i)                                  klinst_AI(KLOPCODE_EQI, (a), (i))
#define klinst_nei(a, i)                                  klinst_AI(KLOPCODE_NEI, (a), (i))
#define klinst_lei(a, i)                                  klinst_AI(KLOPCODE_LEI, (a), (i))
#define klinst_gei(a, i)                                  klinst_AI(KLOPCODE_GEI, (a), (i))
#define klinst_lti(a, i)                                  klinst_AI(KLOPCODE_LTI, (a), (i))
#define klinst_gti(a, i)                                  klinst_AI(KLOPCODE_GTI, (a), (i))
#define klinst_match(a, x)                                klinst_AX(KLOPCODE_MATCH, (a), (x))
#define klinst_parrextra(nback, offset)                   klinst_extra_xi((nback), (offset))
#define klinst_pbarr(a, b, nfront)                        klinst_ABX(KLOPCODE_PBARR, (a), (b), (nfront))
#define klinst_pbtup(a, b, nwanted)                       klinst_ABX(KLOPCODE_PBTUP, (a), (b), (nwanted))
#define klinst_pbmap(a, b, nwanted)                       klinst_ABX(KLOPCODE_PBMAP, (a), (b), (nwanted))
#define klinst_pbobj(a, b, nwanted)                       klinst_ABX(KLOPCODE_PBOBJ, (a), (b), (nwanted))
#define klinst_pmarr(a, b, nfront)                        klinst_ABX(KLOPCODE_PMARR, (a), (b), (nfront))
#define klinst_pmtup(a, b, nwanted)                       klinst_ABX(KLOPCODE_PMTUP, (a), (b), (nwanted))
#define klinst_pmmap(a, b, nwanted)                       klinst_ABX(KLOPCODE_PMMAP, (a), (b), (nwanted))
#define klinst_pmobj(a, b, nwanted)                       klinst_ABX(KLOPCODE_PMOBJ, (a), (b), (nwanted))
#define klinst_pmappost(offset)                           klinst_XI(KLOPCODE_PMAPPOST, 0, (offset))
#define klinst_newobj(a, klclass)                         klinst_ABC(KLOPCODE_NEWOBJ, (a), (klclass), 0)
#define klinst_adjustargs()                               klinst_(KLOPCODE_ADJUSTARGS)
#define klinst_vforprep(a, offset)                        klinst_AI(KLOPCODE_VFORPREP, (a), (offset))
#define klinst_vforloop(a, offset)                        klinst_AI(KLOPCODE_VFORLOOP, (a), (offset))
#define klinst_iforprep(a, offset)                        klinst_AI(KLOPCODE_IFORPREP, (a), (offset))
#define klinst_iforloop(a, offset)                        klinst_AI(KLOPCODE_IFORLOOP, (a), (offset))
#define klinst_gforloop(a, nret)                          klinst_AX(KLOPCODE_GFORLOOP, (a), (nret))
#define klinst_async(a, f)                                klinst_ABC(KLOPCODE_ASYNC, (a), (f), (0))
#define klinst_yield(first, nres, nwanted)                klinst_AXY(KLOPCODE_YIELD, (first), (nres), (nwanted))
#define klinst_vararg(a, nwanted)                         klinst_AXY(KLOPCODE_VARARG, (a), (nwanted), (0))
#define klinst_gforloopextra(offset)                      klinst_extra_i((offset))
#define klinst_extra_xyz(x, y, z)                         klinst_XYZ(KLOPCODE_EXTRA, (x), (y), (z))
#define klinst_extra_xi(x, imm)                           klinst_XI(KLOPCODE_EXTRA, (x), (imm))
#define klinst_extra_i(imm)                               klinst_I(KLOPCODE_EXTRA, (imm))


#define klinst_foreach(macro)                             \
macro(KLOPCODE_MOVE)                                      \
macro(KLOPCODE_MULTIMOVE)                                 \
macro(KLOPCODE_ADD)                                       \
macro(KLOPCODE_SUB)                                       \
macro(KLOPCODE_MUL)                                       \
macro(KLOPCODE_DIV)                                       \
macro(KLOPCODE_MOD)                                       \
macro(KLOPCODE_IDIV)                                      \
macro(KLOPCODE_CONCAT)                                    \
macro(KLOPCODE_ADDI)                                      \
macro(KLOPCODE_SUBI)                                      \
macro(KLOPCODE_MULI)                                      \
macro(KLOPCODE_DIVI)                                      \
macro(KLOPCODE_MODI)                                      \
macro(KLOPCODE_IDIVI)                                     \
macro(KLOPCODE_ADDC)                                      \
macro(KLOPCODE_SUBC)                                      \
macro(KLOPCODE_MULC)                                      \
macro(KLOPCODE_DIVC)                                      \
macro(KLOPCODE_MODC)                                      \
macro(KLOPCODE_IDIVC)                                     \
macro(KLOPCODE_NEG)                                       \
macro(KLOPCODE_LEN)                                       \
macro(KLOPCODE_SCALL)                                     \
macro(KLOPCODE_CALL)                                      \
macro(KLOPCODE_METHOD)                                    \
macro(KLOPCODE_RETURN)                                    \
macro(KLOPCODE_RETURN0)                                   \
macro(KLOPCODE_RETURN1)                                   \
macro(KLOPCODE_LOADBOOL)                                  \
macro(KLOPCODE_LOADI)                                     \
macro(KLOPCODE_LOADC)                                     \
macro(KLOPCODE_LOADNIL)                                   \
macro(KLOPCODE_LOADREF)                                   \
macro(KLOPCODE_LOADGLOBAL)                                \
macro(KLOPCODE_STOREREF)                                  \
macro(KLOPCODE_STOREGLOBAL)                               \
macro(KLOPCODE_MKTUPLE)                                   \
macro(KLOPCODE_MKMAP)                                     \
macro(KLOPCODE_MKARRAY)                                   \
macro(KLOPCODE_MKCLOSURE)                                 \
macro(KLOPCODE_APPEND)                                    \
macro(KLOPCODE_MKCLASS)                                   \
macro(KLOPCODE_INDEXI)                                    \
macro(KLOPCODE_INDEXASI)                                  \
macro(KLOPCODE_INDEX)                                     \
macro(KLOPCODE_INDEXAS)                                   \
macro(KLOPCODE_GETFIELDR)                                 \
macro(KLOPCODE_GETFIELDC)                                 \
macro(KLOPCODE_SETFIELDR)                                 \
macro(KLOPCODE_SETFIELDC)                                 \
macro(KLOPCODE_REFGETFIELDR)                              \
macro(KLOPCODE_REFGETFIELDC)                              \
macro(KLOPCODE_REFSETFIELDR)                              \
macro(KLOPCODE_REFSETFIELDC)                              \
macro(KLOPCODE_NEWLOCAL)                                  \
macro(KLOPCODE_NEWMETHODC)                                \
macro(KLOPCODE_NEWMETHODR)                                \
macro(KLOPCODE_LOADFALSESKIP)                             \
macro(KLOPCODE_TESTSET)                                   \
macro(KLOPCODE_TRUEJMP)                                   \
macro(KLOPCODE_FALSEJMP)                                  \
macro(KLOPCODE_JMP)                                       \
macro(KLOPCODE_CONDJMP)                                   \
macro(KLOPCODE_CLOSEJMP)                                  \
macro(KLOPCODE_HASFIELD)                                  \
macro(KLOPCODE_IS)                                        \
macro(KLOPCODE_EQ)                                        \
macro(KLOPCODE_NE)                                        \
macro(KLOPCODE_LE)                                        \
macro(KLOPCODE_GE)                                        \
macro(KLOPCODE_LT)                                        \
macro(KLOPCODE_GT)                                        \
macro(KLOPCODE_EQC)                                       \
macro(KLOPCODE_NEC)                                       \
macro(KLOPCODE_LEC)                                       \
macro(KLOPCODE_GEC)                                       \
macro(KLOPCODE_LTC)                                       \
macro(KLOPCODE_GTC)                                       \
macro(KLOPCODE_EQI)                                       \
macro(KLOPCODE_NEI)                                       \
macro(KLOPCODE_LEI)                                       \
macro(KLOPCODE_GEI)                                       \
macro(KLOPCODE_LTI)                                       \
macro(KLOPCODE_GTI)                                       \
macro(KLOPCODE_MATCH)                                     \
macro(KLOPCODE_PBARR)                                     \
macro(KLOPCODE_PBTUP)                                     \
macro(KLOPCODE_PBMAP)                                     \
macro(KLOPCODE_PBOBJ)                                     \
macro(KLOPCODE_PMARR)                                     \
macro(KLOPCODE_PMTUP)                                     \
macro(KLOPCODE_PMMAP)                                     \
macro(KLOPCODE_PMOBJ)                                     \
macro(KLOPCODE_PMAPPOST)                                  \
macro(KLOPCODE_NEWOBJ)                                    \
macro(KLOPCODE_ADJUSTARGS)                                \
macro(KLOPCODE_VFORPREP)                                  \
macro(KLOPCODE_VFORLOOP)                                  \
macro(KLOPCODE_IFORPREP)                                  \
macro(KLOPCODE_IFORLOOP)                                  \
macro(KLOPCODE_GFORLOOP)                                  \
macro(KLOPCODE_ASYNC)                                     \
macro(KLOPCODE_YIELD)                                     \
macro(KLOPCODE_VARARG)



#endif
