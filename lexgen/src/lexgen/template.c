#include "lexgen/include/lexgen/template.h"

static void kev_template_replace(FILE* output, FILE* tmpl, KevStringMap* tmpl_map);
static void kev_template_convert_plain(FILE* output, FILE* tmpl, KevStringMap* tmpl_map);
static void kev_template_condition(FILE* output, FILE* tmpl, KevStringMap* tmpl_map);
static void kev_template_replace_no_output(FILE* tmpl);
static void kev_template_convert_plain_no_output(FILE* tmpl);
static void kev_template_condition_no_output(FILE* tmpl);

static bool kev_template_expr_or(FILE* tmpl, KevStringMap* tmpl_map);
static bool kev_template_expr_and(FILE* tmpl, KevStringMap* tmpl_map);
static bool kev_template_expr_unit(FILE* tmpl, KevStringMap* tmpl_map);

static int kev_template_read_id(FILE* tmpl, char* buf, size_t buf_size);
static int kev_template_next_non_blank(FILE* tmpl);
static void fatal_error(char* info, char* info2);

void kev_template_convert(FILE* output, FILE* tmpl, KevStringMap* tmpl_map) {
  kev_template_convert_plain(output, tmpl, tmpl_map);
  if (fgetc(tmpl) != EOF)
    fatal_error("template: trainling text", NULL);
}

static void kev_template_convert_plain(FILE* output, FILE* tmpl, KevStringMap* tmpl_map) {
  int ch = 0;
  while ((ch = fgetc(tmpl)) != EOF) {
    if (ch == '$') {
      ch = fgetc(tmpl);
      switch (ch) {
        case 'r': {
          kev_template_replace(output, tmpl, tmpl_map);
          break;
        }
        case 'c': {
          kev_template_condition(output, tmpl, tmpl_map);
          break;
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
          ungetc(ch, tmpl);
          ungetc('$', tmpl);
          return;
        }
      }
    } else {
      fputc(ch, output);
    }
  }
}

static void kev_template_replace(FILE* output, FILE* tmpl, KevStringMap* tmpl_map) {
  char buffer[1024];
  int ch = kev_template_read_id(tmpl, buffer, 1024);
  KevStringMapNode* node = kev_strmap_search(tmpl_map, buffer);
  if (node) {
    fputs(node->value, output);
  }
  while (ch == ' ' || ch == '\t' || ch == '\n')
    ch = fgetc(tmpl);
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
  int ch = kev_template_read_id(tmpl, buffer, 1024);
  while (ch == ' ' || ch == '\t' || ch == '\n')
    ch = fgetc(tmpl);
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
  int ch = 0;
  while ((ch = fgetc(tmpl)) != EOF) {
    if (ch == '$') {
      ch = fgetc(tmpl);
      switch (ch) {
        case 'r': {
          kev_template_replace_no_output(tmpl);
          break;
        }
        case 'c': {
          kev_template_condition_no_output(tmpl);
          break;
        }
        case '$': {
          break;
        }
        case ' ': case '\t': case '\n': {
          break;
        }
        default: {
          ungetc(ch, tmpl);
          ungetc('$', tmpl);
          return;
        }
      }
    }
  }
}

static void fatal_error(char* info, char* info2) {
  if (info) fputs(info, stderr);
  if (info2) fputs(info2, stderr);
  fputs("\nterminated\n", stderr);
}

static int kev_template_read_id(FILE* tmpl, char* buf, size_t buf_size) {
  int ch = kev_template_next_non_blank(tmpl);
  size_t i = 0;
  if (!(((ch | 0x20) <= 'z' && (ch | 0x20) >= 'a') || ch == '_'))
    fatal_error("template: illegal identifier", NULL);
  do {
    buf[i++] = ch;
  } while (((((ch = fgetc(tmpl)) | 0x20) <= 'z' && (ch | 0x20) >= 'a') ||
           (ch <= '9' && ch >= '0') || ch == '_' || ch == '-') && i < buf_size);
  if (i == buf_size) {
    buf[buf_size - 1] = '\0';
    fatal_error("template: identifier too long: ", buf);
  }
  buf[i] = '\0';
  return ch;
}

static void kev_template_condition(FILE* output, FILE* tmpl, KevStringMap* tmpl_map) {
  bool val = kev_template_expr_or(tmpl, tmpl_map);
  int ch = kev_template_next_non_blank(tmpl);
  if (ch != '$' || fgetc(tmpl) != ':')
    fatal_error("template: expected $:", NULL);
  if (val) {
    kev_template_convert_plain(output, tmpl, tmpl_map);
  } else {
    kev_template_convert_plain_no_output(tmpl);
  }
  ch = kev_template_next_non_blank(tmpl);
  if (ch != '$')
    fatal_error("template: expected $; or expected $|", NULL);
  if ((ch = fgetc(tmpl)) == '|') {
    if (val) {
      kev_template_convert_plain_no_output(tmpl);
    } else {
      kev_template_convert_plain(output, tmpl, tmpl_map);
    }
    if (fgetc(tmpl) != '$' || fgetc(tmpl) != ';')
      fatal_error("template: expected $;", NULL);
  }
  else if (ch != ';') {
    fatal_error("template: expected $;", NULL);
  }
}

static void kev_template_condition_no_output(FILE* tmpl) {
  kev_template_expr_or(tmpl, NULL);
  int ch = kev_template_next_non_blank(tmpl);
  if (ch != '$' || fgetc(tmpl) != ':')
    fatal_error("template: expected $:", NULL);
  kev_template_convert_plain_no_output(tmpl);
  ch = kev_template_next_non_blank(tmpl);
  if (ch != '$')
    fatal_error("template: expected $; or expected $|", NULL);
  if ((ch = fgetc(tmpl)) == '|') {
      kev_template_convert_plain_no_output(tmpl);
    if (fgetc(tmpl) != '$' || fgetc(tmpl) != ';')
      fatal_error("template: expected $;", NULL);
  }
  else if (ch != ';') {
    fatal_error("template: expected $;", NULL);
  }
}

static bool kev_template_expr_or(FILE* tmpl, KevStringMap* tmpl_map) {
  bool value = false;
  int ch = '|';
  while (ch == '|') {
    value = kev_template_expr_and(tmpl, tmpl_map) || value;
    ch = kev_template_next_non_blank(tmpl);
  }
  ungetc(ch, tmpl);
  return value;
}

static bool kev_template_expr_and(FILE* tmpl, KevStringMap* tmpl_map) {
  bool value = true;
  int ch = '&';
  while (ch == '&') {
    value = kev_template_expr_unit(tmpl, tmpl_map) && value;
    ch = kev_template_next_non_blank(tmpl);
  }
  ungetc(ch, tmpl);
  return value;
}

static bool kev_template_expr_unit(FILE* tmpl, KevStringMap* tmpl_map) {
  int ch = kev_template_next_non_blank(tmpl);
  if (ch == '!') {
    return !kev_template_expr_unit(tmpl, tmpl_map);
  } else if (ch == '(') {
    bool value = kev_template_expr_or(tmpl, tmpl_map);
    if (fgetc(tmpl) != ')')
      fatal_error("template: missing \')\'", NULL);
    return value;
  } else {
    char buffer[1024];
    ungetc(ch, tmpl);
    ch = kev_template_read_id(tmpl, buffer, 1024);
    ungetc(ch, tmpl);
    if (!tmpl_map) return false;
    return kev_strmap_search(tmpl_map, buffer) != NULL;
  }
}

static int kev_template_next_non_blank(FILE* tmpl) {
  int ch = 0;
  while ((ch = fgetc(tmpl)) == ' ' || ch == '\t' || ch == '\n')
    continue;
  return ch;
}
