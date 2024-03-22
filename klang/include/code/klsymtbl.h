#ifndef KEVCC_KLANG_INCLUDE_CODE_KLSYMTBL_H
#define KEVCC_KLANG_INCLUDE_CODE_KLSYMTBL_H

#include "klang/include/parse/klstrtab.h"

typedef enum tagKlSymDuration {
  KLSYMDUR_STACK,
  KLSYMDUR_REF,
} KlSymDuration;

typedef struct tagKlSymbolAttr {
  size_t pos;
  KlSymDuration duration;
} KlSymbolAttr;

typedef struct tagKlSymbol KlSymbol;
struct tagKlSymbol {
  size_t hash;
  KlStrDesc name;
  KlSymbolAttr attr;
  KlSymbol* next;
};

typedef struct tagKlSymTbl {
  KlSymbol** array;
  size_t capacity;
  size_t size;
  KlStrTab* strtab;
} KlSymTbl;


bool klsymtbl_init(KlSymTbl* symtbl, size_t capacity, KlStrTab* strtab);
void klsymtbl_destroy(KlSymTbl* symtbl);
KlSymTbl* klsymtbl_create(size_t capacity, KlStrTab* strtab);
void klsymtbl_delete(KlSymTbl* symtbl);

KlSymbol* klsymtbl_insert(KlSymTbl* symtbl, KlStrDesc name);
KlSymbol* klsymtbl_search(KlSymTbl* map, KlStrDesc name);

#endif
