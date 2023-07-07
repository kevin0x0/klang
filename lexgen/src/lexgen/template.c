#include "lexgen/include/lexgen/template.h"

void kev_template_convert(FILE* output, FILE* tmpl, KevStringMap* tmpl_map) {
  int ch = 0;
  while ((ch = fgetc(tmpl)) != EOF) {
    if (ch == '$') {
      static char name[1024];
      size_t pos = 0;
      while ((ch = fgetc(tmpl)) != '$' && pos < 1023 && ch != EOF) {
        if (ch == '\\') ch = fgetc(tmpl);
        name[pos++] = ch;
      }
      if (ch == EOF)
        fprintf(stderr, "fatal: template file is corrupted\nterminated\n");
      name[pos] = '\0';
      KevStringMapNode* node = kev_strmap_search(tmpl_map, name);
      fprintf(output, "%s", node ? node->value : name);
    } else {
      fputc(ch, output);
    }
  }
}
