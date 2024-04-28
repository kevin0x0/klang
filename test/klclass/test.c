#include "klang/include/mm/klmm.h"
#include "klang/include/klapi.h"
#include "klang/include/value/klclass.h"

#include <stdio.h>
#include <string.h>



int main(void) {
  KlMM klmm;
  klmm_init(&klmm, 1024);
  KlState* K = klapi_new_state(&klmm);

  klapi_pushnil(K, 100);
  klapi_pop(K, 100);
  KlClass* Person = klclass_create(&klmm, 1, KLOBJECT_DEFAULT_ATTROFF, NULL, NULL);
  klapi_pushobj(K, Person, KL_CLASS);
  char key[100];
  char value[100];
  char modi[100];
  FILE* fin = fopen("test.txt", "r");
  while (true) {
    fscanf(fin, "%s %s %s", modi, key, value);
    if (feof(fin)) {
      fclose(fin);
      fin = stdin;
      break;
    }
    if (strcmp(modi, "shared") == 0) {
      klapi_pushstring(K, key);
      klapi_pushstring(K, value);
      klclass_newshared(Person, &klmm, klapi_getstring(K, -2), klapi_access(K, -1));
      klapi_pop(K, 2);
    } else {
      klapi_pushstring(K, key);
      klclass_newlocal(Person, &klmm, klapi_getstring(K, -1));
      klapi_pop(K, 1);
    }
  }
  KlValue val;
  klclass_new_object(Person, &klmm, &val);
  KlObject* person = val.value.gcobj;
  klapi_pushobj(K, person, KL_OBJECT);
  while (true) {
    char cmd[100];
    char buf[100];
    fscanf(fin, "%s %s", cmd, buf);
    if (strcmp(cmd, "show") == 0) {
      KlValue* val = klobject_getfield(person, klstrpool_new_string(K->strpool, buf));
      if (!val) {
        fprintf(stderr, "no this attribute\n");
        continue;
      }
      if (klvalue_checktype(val, KL_NIL)) {
        fprintf(stdout, "nil\n");
      } else {
        fprintf(stdout, "%s\n", klstring_content(klvalue_getobj(val, KlString*)));
      }
    } else {
      KlValue* val = klobject_getfield(person, klstrpool_new_string(K->strpool, buf));
      if (!val) {
        fprintf(stderr, "no this attribute\n");
        continue;
      }
      fscanf(fin, "%s", buf);
      klvalue_setobj(val, klstrpool_new_string(K->strpool, buf), KL_STRING);
    }
  }
  klmm_destroy(&klmm);
  return 0;
}
