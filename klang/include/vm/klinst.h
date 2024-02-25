#ifndef KEVCC_KLANG_INCLUDE_VM_KLINST_H
#define KEVCC_KLANG_INCLUDE_VM_KLINST_H

#include "klang/include/misc/klutils.h"
#include <stdint.h>


typedef uint32_t KlInstruction;


/* A, B, C, ...         represent register index
 * X, Y, Z, U, V, ...   represent unsigned immediate number
 * I, J, K, ...         represent signed immediate number
 * T                    represent tag(can be tested)
 */

#define klinst_utos(num, width)         ((int32_t)(num) - (klbit((width) - 1)))
#define klinst_stou(num, width)         ((int32_t)(num) + (klbit((width) - 1)))

#define KLINST_GET_OPCODE(inst)         ((uint8_t)(inst))

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
  ((kllow8bits(opcode)) | (kllow8bits(a) << 8) | ((uint8_t)(b) << 16) | (kllowbits(t, 1) << 24) | (kllowbits(c, 7) << 25))
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
#define KLINST_AI_GETI(inst)            (klinst_utos((inst) >> 16, 16))





#define KLOPCODE_MOVE               (0)
#define KLOPCODE_ADD                (1)
#define KLOPCODE_SUB                (2)
#define KLOPCODE_MUL                (3)
#define KLOPCODE_DIV                (4)
#define KLOPCODE_MOD                (5)
#define KLOPCODE_ADDI               (6)
#define KLOPCODE_SUBI               (7)
#define KLOPCODE_MULI               (8)
#define KLOPCODE_DIVI               (9)
#define KLOPCODE_MODI               (10)
#define KLOPCODE_ADDC               (11)
#define KLOPCODE_SUBC               (12)
#define KLOPCODE_MULC               (13)
#define KLOPCODE_DIVC               (14)
#define KLOPCODE_MODC               (15)
#define KLOPCODE_CONCAT             (16)
#define KLOPCODE_NEG                (17)
#define KLOPCODE_CALL               (18)
#define KLOPCODE_METHOD             (19)
#define KLOPCODE_RETURN             (20)
#define KLOPCODE_RETURN0            (21)
#define KLOPCODE_RETURN1            (22)
#define KLOPCODE_LOADBOOL           (23)
#define KLOPCODE_LOADI              (24)
#define KLOPCODE_LOADC              (25)
#define KLOPCODE_LOADNIL            (26)
#define KLOPCODE_LOADREF            (27)
#define KLOPCODE_LOADTHIS           (28)
#define KLOPCODE_STOREREF           (29)
#define KLOPCODE_STORETHIS          (30)
#define KLOPCODE_MKMAP              (31)
#define KLOPCODE_MKARRAY            (32)
#define KLOPCODE_MKCLASS            (33)
#define KLOPCODE_INDEX              (34)
#define KLOPCODE_INDEXAS            (35)
#define KLOPCODE_GETFIELDR          (36)
#define KLOPCODE_GETFIELDC          (37)
#define KLOPCODE_SETFIELDR          (38)
#define KLOPCODE_SETFIELDC          (39)
#define KLOPCODE_NEWLOCAL           (40)
#define KLOPCODE_JMP                (41)
#define KLOPCODE_BE                 (42)
#define KLOPCODE_BNE                (43)
#define KLOPCODE_BLE                (44)
#define KLOPCODE_BGE                (45)
#define KLOPCODE_BL                 (46)
#define KLOPCODE_BG                 (47)
#define KLOPCODE_BEC                (48)
#define KLOPCODE_BNEC               (49)
#define KLOPCODE_BLEC               (50)
#define KLOPCODE_BGEC               (51)
#define KLOPCODE_BLC                (52)
#define KLOPCODE_BGC                (53)
#define KLOPCODE_BEI                (54)
#define KLOPCODE_BNEI               (55)
#define KLOPCODE_BLEI               (56)
#define KLOPCODE_BGEI               (57)
#define KLOPCODE_BLI                (58)
#define KLOPCODE_BGI                (59)
#define KLOPCODE_CLOSE              (60)
#define KLOPCODE_EXTRA              (255)


#define klinst_move(a, b)                                 (klinst_ABC(KLOPCODE_MOVE, (a), (b), 0))
#define klinst_add(a, b, c)                               (klinst_ABC(KLOPCODE_ADD, (a), (b), (c)))
#define klinst_sub(a, b, c)                               (klinst_ABC(KLOPCODE_SUB, (a), (b), (c)))
#define klinst_mul(a, b, c)                               (klinst_ABC(KLOPCODE_MUL, (a), (b), (c)))
#define klinst_div(a, b, c)                               (klinst_ABC(KLOPCODE_DIV, (a), (b), (c)))
#define klinst_mod(a, b, c)                               (klinst_ABC(KLOPCODE_MOD, (a), (b), (c)))
#define klinst_addi(a, b, i)                              (klinst_ABI(KLOPCODE_ADDI, (a), (b), (i)))
#define klinst_subi(a, b, i)                              (klinst_ABI(KLOPCODE_SUBI, (a), (b), (i)))
#define klinst_muli(a, b, i)                              (klinst_ABI(KLOPCODE_MULI, (a), (b), (i)))
#define klinst_divi(a, b, i)                              (klinst_ABI(KLOPCODE_DIVI, (a), (b), (i)))
#define klinst_modi(a, b, i)                              (klinst_ABI(KLOPCODE_MODI, (a), (b), (i)))
#define klinst_addc(a, b, x)                              (klinst_ABX(KLOPCODE_ADDC, (a), (b), (x)))
#define klinst_subc(a, b, x)                              (klinst_ABX(KLOPCODE_SUBC, (a), (b), (x)))
#define klinst_mulc(a, b, x)                              (klinst_ABX(KLOPCODE_MULC, (a), (b), (x)))
#define klinst_divc(a, b, x)                              (klinst_ABX(KLOPCODE_DIVC, (a), (b), (x)))
#define klinst_modc(a, b, x)                              (klinst_ABX(KLOPCODE_MODC, (a), (b), (x)))
#define klinst_concat(a, first, nelem)                    (klinst_ABX(KLOPCODE_CONCAT, (a), (first), (nelem)))
#define klinst_neg(a, b)                                  (klinst_ABC(KLOPCODE_NEG, (a,) (b), 0))
#define klinst_call(callable, narg, nret)                 (klinst_AXY(KLOPCODE_CALL, (callable), (narg), (nret)))
#define klinst_method(thisobj, callable, narg, nret)      
#define klinst_return(first, nres)                        (klinst_AX(KLOPCODE_RETURN, (first), (nret)))
#define klinst_return0()                                  (klinst_(KLOPCODE_RETURN0))
#define klinst_return1(res)                               (klinst_A(KLOPCODE_RETURN1, (res)))
#define klinst_loadbool(a, x)                             (klinst_AX(KLOPCODE_LOADBOOL, (a), (x)))
#define klinst_loadi(a, i)                                (klinst_AI(KLOPCODE_LOADI, (a), (i)))
#define klinst_loadc(a, x)                                (klinst_AX(KLOPCODE_LOADC, (a), (x)))
#define klinst_loadnil(a, count)                          (klinst_AX(KLOPCODE_LOADNIL, (a), (count)))
#define klinst_loadref(a, refidx)                         (klinst_AX(KLOPCODE_LOADREF, (a), (refidx)))
#define klinst_loadthis(a)                                (klinst_A(KLOPCODE_LOADTHIS, (a)))
#define klinst_storeref(a, refidx)                        (klinst_AX(KLOPCODE_LOADREF, (a), (refidx)))
#define klinst_storethis(a)                               (klinst_A(KLOPCODE_LOADTHIS, (a)))
#define klinst_mkmap(a, stktop, capacity)                 (klinst_ABX(KLOPCODE_MKMAP, (a), (stktop), (capacity)))
#define klinst_mkarray(a, first, nelem)                   (klinst_ABX(KLOPCODE_MKARRAY, (a), (first), (nelem)))
#define klinst_mkclass(a, stktop, topisbase, capacity)    (klinst_ABTX(KLOPCODE_MKCLASS, (a), (stktop), (topisbase), (capacity)))
#define klinst_index(a, indexable, key)                   (klinst_ABC(KLOPCODE_INDEX, (a), (indexable), (key)))
#define klinst_indexas(val, indexable, key)               (klinst_ABC(KLOPCODE_INDEXAS, (val), (indexable), (key)))
#define klinst_getfieldr(a, obj, field)                   (klinst_ABC(KLOPCODE_GETFIELDR, (a), (obj), (field)))
#define klinst_getfieldc(a, obj, field)                   (klinst_ABX(KLOPCODE_GETFIELDC, (a), (obj), (field)))
#define klinst_setfieldr(a, obj, field)                   (klinst_ABC(KLOPCODE_SETFIELDR, (a), (obj), (field)))
#define klinst_setfieldc(a, obj, field)                   (klinst_ABX(KLOPCODE_SETFIELDC, (a), (obj), (field)))
#define klinst_newlocal(a, x)                             (klinst_AX(KLOPCODE_NEWLOCAL, (a), (x)))
#define klinst_jmp(offset)                                (klinst_I(KLOPCODE_JMP, (offset)))
#define klinst_be(a, b, offset)                           (klinst_ABI(KLOPCODE_BE, (a), (b), (offset)))
#define klinst_bne(a, b, offset)                          (klinst_ABI(KLOPCODE_BNE, (a), (b), (offset)))
#define klinst_ble(a, b, offset)                          (klinst_ABI(KLOPCODE_BLE, (a), (b), (offset)))
#define klinst_bge(a, b, offset)                          (klinst_ABI(KLOPCODE_BGE, (a), (b), (offset)))
#define klinst_bl(a, b, offset)                           (klinst_ABI(KLOPCODE_BL, (a), (b), (offset)))
#define klinst_bg(a, b, offset)                           (klinst_ABI(KLOPCODE_BG, (a), (b), (offset)))
#define klinst_bec(a, x, offset)                          (klinst_AXI(KLOPCODE_BEC, (a), (x), (offset)))
#define klinst_bnec(a, x, offset)                         (klinst_AXI(KLOPCODE_BNEC, (a), (x), (offset)))
#define klinst_blec(a, x, offset)                         (klinst_AXI(KLOPCODE_BLEC, (a), (x), (offset)))
#define klinst_bgec(a, x, offset)                         (klinst_AXI(KLOPCODE_BGEC, (a), (x), (offset)))
#define klinst_blc(a, x, offset)                          (klinst_AXI(KLOPCODE_BLC, (a), (x), (offset)))
#define klinst_bgc(a, x, offset)                          (klinst_AXI(KLOPCODE_BGC, (a), (x), (offset)))
#define klinst_bei(a, i, offset)                          (klinst_AIJ(KLOPCODE_BEI, (a), (i), (offset)))
#define klinst_bnei(a, i, offset)                         (klinst_AIJ(KLOPCODE_BNEI, (a), (i), (offset)))
#define klinst_blei(a, i, offset)                         (klinst_AIJ(KLOPCODE_BLEI, (a), (i), (offset)))
#define klinst_bgei(a, i, offset)                         (klinst_AIJ(KLOPCODE_BGEI, (a), (i), (offset)))
#define klinst_bli(a, i, offset)                          (klinst_AIJ(KLOPCODE_BLI, (a), (i), (offset)))
#define klinst_bgi(a, i, offset)                          (klinst_AIJ(KLOPCODE_BGI, (a), (i), (offset)))
#define klinst_close(bound)                               (klinst_X(KLOPCODE_CLOSE, (bound)))
#define klinst_extra(...)                                 (klinst_XYZ(KLOPCODE_EXTRA, (x), (y), (z)))





#endif
