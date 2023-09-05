#include "lexgen/include/lexgen/template.h"

#include <stdio.h>

int main(int argc, char** argv) {
  FILE* tmpl = fopen("test.tmpl", "r");
  FILE* output = fopen("test.txt", "w");
  KevStringMap* map = kev_strmap_create(8);
  kev_strmap_update(map, "output_name", "Wang Zhongzheng");
  kev_strmap_update(map, "output_age", "Wang Zhongzheng");
  kev_template_convert(output, tmpl, map);
  fclose(tmpl);
  fclose(output);
  kev_strmap_delete(map);
  return 0;
}
