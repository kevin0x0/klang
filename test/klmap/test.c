#include "klang/include/value/klmap.h"
#include "klang/include/value/value.h"
#include <stdio.h>

int main(void) {
  while (true) {
    KlMap* map = klmap_create(8);
    KlValue* val0 = klvalue_create_int(0);
    KlValue* val1 = klvalue_create_int(1);
    KlValue* val2 = klvalue_create_int(2);
    KlValue* val3 = klvalue_create_int(3);
    KlValue* val4 = klvalue_create_int(4);
    KlValue* val5 = klvalue_create_int(5);
    KlValue* val6 = klvalue_create_int(6);
    KlValue* val7 = klvalue_create_int(7);
    KlValue* val8 = klvalue_create_int(8);
    KlValue* val9 = klvalue_create_int(9);
    KlValue* val10 = klvalue_create_int(10);
    KlValue* val11 = klvalue_create_int(11);
    KlValue* val12 = klvalue_create_int(12);
    klmap_insert_move(map, kstring_create("0"), val0);
    for (KlMapIter iter = klmap_iter_begin(map); iter != klmap_iter_end(map); /* iter = klmap_iter_next(iter) */) {
      iter = klmap_erase(map, iter);
    }
    klmap_insert_move(map, kstring_create("1"), val1);
    klmap_insert_move(map, kstring_create("2"), val2);
    klmap_insert_move(map, kstring_create("3"), val3);
    klmap_insert_move(map, kstring_create("4"), val4);
    klmap_insert_move(map, kstring_create("5"), val5);
    klmap_insert_move(map, kstring_create("6"), val6);
    klmap_insert_move(map, kstring_create("7"), val7);
    klmap_insert_move(map, kstring_create("8"), val8);
    klmap_insert_move(map, kstring_create("9"), val9);
    klmap_insert_move(map, kstring_create("10"), val10);
    klmap_insert_move(map, kstring_create("11"), val11);
    klmap_insert_move(map, kstring_create("12"), val12);

    klmap_delete(map);
    klvalue_delete(val0);
    klvalue_delete(val1);
    klvalue_delete(val2);
    klvalue_delete(val3);
    klvalue_delete(val4);
    klvalue_delete(val5);
    klvalue_delete(val6);
    klvalue_delete(val7);
    klvalue_delete(val8);
    klvalue_delete(val9);
    klvalue_delete(val10);
    klvalue_delete(val11);
    klvalue_delete(val12);
  }
  return 0;
}
