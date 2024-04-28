#include "klang/include/parse/klsymtab.h"
#include <stdio.h>

int main(void) {
  while (true) {
    KlSymTab* symtab = klsymtab_create(NULL);

    klsymtab_insert_move(symtab, kstring_create("Hello0"));
    klsymtab_insert_move(symtab, kstring_create("Hello1"));
    klsymtab_insert_move(symtab, kstring_create("Hello2"));
    klsymtab_insert_move(symtab, kstring_create("Hello3"));
    klsymtab_insert_move(symtab, kstring_create("Hello4"));

    for (KlSymTabIter itr = klsymtab_iter_begin(symtab); itr != klsymtab_iter_end(symtab); ) {
      printf("%s\n", kstring_get_content(itr->name));
      itr = klsymtab_erase(symtab, itr);
    }

    klsymtab_delete(symtab);
  }

  return 0;
}
