#include "pargen/include/parser/symtable.h"

#include <stdlib.h>


KevSymTable* kev_symtable_create(size_t self, size_t parent) {
  KevSymTable* symtable = (KevSymTable*)malloc(sizeof (KevSymTable));
  if (!symtable || kev_strmap_init(&symtable->table, 8)) {
    free(symtable);
    return NULL;
  }
  symtable->self = self;
  symtable->parent = parent;
  return symtable;
}

void kev_symtable_delete(KevSymTable* symtable) {
  if (symtable) {
    kev_strmap_destroy(&symtable->table);
    free(symtable);
  }
}
