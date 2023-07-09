#include "lexgen/include/lexgen/template.h"
#include "lexgen/include/parser/hashmap/str_map.h"

#include <stdio.h>

int main(int argc, char** argv) {
  FILE* tmpl = fopen("test.tmpl", "r");
  FILE* output = fopen("test.txt", "w");
  KevStringMap* map = kev_strmap_create(8);
  kev_strmap_update(map, "output", "Hello, Kevin Jay!");
  kev_strmap_update(map, "greeting", "Fuck you");
  kev_strmap_update(map, "name", "Wang Zhongzheng");
  kev_template_convert(output, tmpl, map);
  fclose(tmpl);
  fclose(output);
  kev_strmap_delete(map);
  return 0;
}
