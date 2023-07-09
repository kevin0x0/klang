#include "lexgen/include/lexgen/template.h"

static void fatal_error(char* info, char* info2);
static void kev_template_replace(FILE* output, FILE* tmpl, KevStringMap* tmpl_map);
static void kev_template_convert_plain(FILE* output, FILE* tmpl, KevStringMap* tmpl_map);
static void kev_template_replace_no_output(FILE* tmpl);
static void kev_template_convert_plain_no_output(FILE* tmpl);

void kev_template_convert(FILE* output, FILE* tmpl, KevStringMap* tmpl_map) {
  kev_template_convert_plain(output, tmpl, tmpl_map);
  if (fgetc(tmpl) != EOF)
    fatal_error("template: trainling text", NULL);
}

void kev_template_convert_plain(FILE* output, FILE* tmpl, KevStringMap* tmpl_map) {
  char buffer[1024];
  int ch = 0;
  while ((ch = fgetc(tmpl)) != EOF) {
    if (ch == '$') {
      ch = fgetc(tmpl);
      switch (ch) {
        case 'r': {
          kev_template_replace(output, tmpl, tmpl_map);
          break;
        }
        case ';': {
          ungetc(';', tmpl);
          ungetc('$', tmpl);
          return;
        }
        case '$': {
          fputc('$', output);
          break;
        }
        case ' ': case '\t': case '\n': {
          fputc(ch, output);
          break;
        }
        default: {
          buffer[0] = '$';
          buffer[1] = ch;
          buffer[2] = '\0';
          fatal_error("template: unexpected token: ", buffer);
        }
      }
    } else {
      fputc(ch, output);
    }
  }
}

static void kev_template_replace(FILE* output, FILE* tmpl, KevStringMap* tmpl_map) {
  char buffer[1024];
  int ch = 0;
  while ((ch = fgetc(tmpl)) == ' ' || ch == '\t' || ch == '\n')
    continue;
  size_t i = 0;
  if (!(((ch | 0x20) <= 'z' && (ch | 0x20) >= 'a') || ch == '_'))
    fatal_error("template: illegal identifier", NULL);
  do {
    buffer[i++] = ch;
  } while ((((ch = fgetc(tmpl)) | 0x20) <= 'z' && (ch | 0x20) >= 'a') ||
           (ch <= '9' && ch >= '0') || ch == '_');
  buffer[i] = '\0';
  KevStringMapNode* node = kev_strmap_search(tmpl_map, buffer);
  if (node) {
    fputs(node->value, output);
  }
  while ((ch = fgetc(tmpl)) == ' ' || ch == '\t' || ch == '\n')
      continue;
  if (ch != '$') {
    buffer[0] = ch;
    buffer[1] = '\0';
    fatal_error("template: unexpected ", buffer);
  }
  if ((ch = fgetc(tmpl)) != ':') {
    buffer[0] = '$';
    buffer[1] = ch;
    buffer[2] = '\0';
    fatal_error("template: unexpected ", buffer);
  }
  if (node)
    kev_template_convert_plain_no_output(tmpl);
  else
    kev_template_convert_plain(output, tmpl, tmpl_map);
  if (fgetc(tmpl) != '$' || fgetc(tmpl) != ';')
    fatal_error("template: missing $;", NULL);
}

static void kev_template_replace_no_output(FILE* tmpl) {
  char buffer[1024];
  int ch = 0;
  while ((ch = fgetc(tmpl)) == ' ' || ch == '\t' || ch == '\n')
    continue;
  size_t i = 0;
  if (!(((ch | 0x20) <= 'z' && (ch | 0x20) >= 'a') || ch == '_'))
    fatal_error("template: illegal identifier", NULL);
  do {
    buffer[i++] = ch;
  } while ((((ch = fgetc(tmpl)) | 0x20) <= 'z' && (ch | 0x20) >= 'a') ||
           (ch <= '9' && ch >= '0') || ch == '_');
  buffer[i] = '\0';
  while ((ch = fgetc(tmpl)) == ' ' || ch == '\t' || ch == '\n')
      continue;
  if (ch != '$') {
    buffer[0] = ch;
    buffer[1] = '\0';
    fatal_error("template: unexpected ", buffer);
  }
  if ((ch = fgetc(tmpl)) != ':') {
    buffer[0] = '$';
    buffer[1] = ch;
    buffer[2] = '\0';
    fatal_error("template: unexpected ", buffer);
  }
  kev_template_convert_plain_no_output(tmpl);
  if (fgetc(tmpl) != '$' || fgetc(tmpl) != ';')
    fatal_error("template: missing $;", NULL);
}

static void kev_template_convert_plain_no_output(FILE* tmpl) {
  char buffer[1024];
  int ch = 0;
  while ((ch = fgetc(tmpl)) != EOF) {
    if (ch == '$') {
      ch = fgetc(tmpl);
      switch (ch) {
        case 'r': {
          kev_template_replace_no_output(tmpl);
          break;
        }
        case ';': {
          ungetc(';', tmpl);
          ungetc('$', tmpl);
          return;
        }
        case '$': {
          break;
        }
        case ' ': case '\t': case '\n': {
          break;
        }
        default: {
          buffer[0] = '$';
          buffer[1] = ch;
          buffer[2] = '\0';
          fatal_error("template: unexpected token: ", buffer);
        }
      }
    }
  }
}

static void fatal_error(char* info, char* info2) {
  if (info) fputs(info, stderr);
  if (info2) fputs(info, stderr);
  fputs("\nterminated\n", stderr);
}
