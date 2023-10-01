#include "template/include/template.h"

#include <stdlib.h>

static void kev_template_replace(FILE* out, FILE* tmpl, KevStringMap* env_var, KevFuncMap* funcs);
static void kev_template_convert_plain(FILE* out, FILE* tmpl, KevStringMap* env_var, KevFuncMap* funcs);
static void kev_template_condition(FILE* out, FILE* tmpl, KevStringMap* env_var, KevFuncMap* funcs);
static void kev_template_replace_no_out(FILE* tmpl);
static void kev_template_convert_plain_no_out(FILE* tmpl);
static void kev_template_condition_no_out(FILE* tmpl);

static bool kev_template_expr_or(FILE* tmpl, KevStringMap* env_var, KevFuncMap* funcs);
static bool kev_template_expr_and(FILE* tmpl, KevStringMap* env_var, KevFuncMap* funcs);
static bool kev_template_expr_unit(FILE* tmpl, KevStringMap* env_var, KevFuncMap* funcs);

static int kev_template_read_id(FILE* tmpl, char* buf, size_t buf_size);
static int kev_template_next_non_blank(FILE* tmpl);



static void kev_throw_error(const char* header, const char* info, const char* additional_info);



void kev_template_convert(FILE* out, FILE* tmpl, KevStringMap* env_var, KevFuncMap* funcs) {
  kev_template_convert_plain(out, tmpl, env_var, funcs);
  if (fgetc(tmpl) != EOF)
    kev_throw_error("template: ", "trainling text", NULL);
}

static void kev_template_convert_plain(FILE* out, FILE* tmpl, KevStringMap* env_var, KevFuncMap* funcs) {
  int ch = 0;
  while ((ch = fgetc(tmpl)) != EOF) {
    if (ch == '$') {
      ch = fgetc(tmpl);
      switch (ch) {
        case 'r': {
          kev_template_replace(out, tmpl, env_var, funcs);
          break;
        }
        case 'c': {
          kev_template_condition(out, tmpl, env_var, funcs);
          break;
        }
        case '$': {
          fputc('$', out);
          break;
        }
        case ' ': case '\t': case '\n': {
          fputc(ch, out);
          break;
        }
        default: {
          ungetc(ch, tmpl);
          ungetc('$', tmpl);
          return;
        }
      }
    } else {
      fputc(ch, out);
    }
  }
}

static void kev_template_replace(FILE* out, FILE* tmpl, KevStringMap* env_var, KevFuncMap* funcs) {
  char buffer[1024];
  int ch = kev_template_read_id(tmpl, buffer, 1024);
  KevStringMapNode* strnode = kev_strmap_search(env_var, buffer);
  KevFuncObject* funcobj = NULL;
  if (strnode) {
    fputs(strnode->value, out);
  } else {
    funcobj = kev_funcmap_search(funcs, buffer);
    if (funcobj) {
      ((void (*)(FILE*, void*, KevStringMap*))funcobj->func)(out, funcobj->object, env_var);
    }
  }
  while (ch == ' ' || ch == '\t' || ch == '\n')
    ch = fgetc(tmpl);
  if (ch != '$') {
    buffer[0] = ch;
    buffer[1] = '\0';
    kev_throw_error("template: ", "unexpected ", buffer);
  }
  if ((ch = fgetc(tmpl)) != ':') {
    buffer[0] = '$';
    buffer[1] = ch;
    buffer[2] = '\0';
    kev_throw_error("template: ", "unexpected ", buffer);
  }
  if (strnode || funcobj)
    kev_template_convert_plain_no_out(tmpl);
  else
    kev_template_convert_plain(out, tmpl, env_var, funcs);
  if (fgetc(tmpl) != '$' || fgetc(tmpl) != ';')
    kev_throw_error("template: ", "missing $;", NULL);
}

static void kev_template_replace_no_out(FILE* tmpl) {
  char buffer[1024];
  int ch = kev_template_read_id(tmpl, buffer, 1024);
  while (ch == ' ' || ch == '\t' || ch == '\n')
    ch = fgetc(tmpl);
  if (ch != '$') {
    buffer[0] = ch;
    buffer[1] = '\0';
    kev_throw_error("template: ", "unexpected ", buffer);
  }
  if ((ch = fgetc(tmpl)) != ':') {
    buffer[0] = '$';
    buffer[1] = ch;
    buffer[2] = '\0';
    kev_throw_error("template: ", "unexpected ", buffer);
  }
  kev_template_convert_plain_no_out(tmpl);
  if (fgetc(tmpl) != '$' || fgetc(tmpl) != ';')
    kev_throw_error("template: ", "missing $;", NULL);
}

static void kev_template_convert_plain_no_out(FILE* tmpl) {
  int ch = 0;
  while ((ch = fgetc(tmpl)) != EOF) {
    if (ch == '$') {
      ch = fgetc(tmpl);
      switch (ch) {
        case 'r': {
          kev_template_replace_no_out(tmpl);
          break;
        }
        case 'c': {
          kev_template_condition_no_out(tmpl);
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

static int kev_template_read_id(FILE* tmpl, char* buf, size_t buf_size) {
  int ch = kev_template_next_non_blank(tmpl);
  size_t i = 0;
  if (!(((ch | 0x20) <= 'z' && (ch | 0x20) >= 'a') || ch == '_'))
    kev_throw_error("template: ", "illegal identifier", NULL);
  do {
    buf[i++] = ch;
  } while (((((ch = fgetc(tmpl)) | 0x20) <= 'z' && (ch | 0x20) >= 'a') ||
           (ch <= '9' && ch >= '0') || ch == '_' || ch == '-') && i < buf_size);
  if (i == buf_size) {
    buf[buf_size - 1] = '\0';
    kev_throw_error("template: ", "identifier too long: ", buf);
  }
  buf[i] = '\0';
  return ch;
}

static void kev_template_condition(FILE* out, FILE* tmpl, KevStringMap* env_var, KevFuncMap* funcs) {
  bool val = kev_template_expr_or(tmpl, env_var, funcs);
  int ch = kev_template_next_non_blank(tmpl);
  if (ch != '$' || fgetc(tmpl) != ':')
    kev_throw_error("template: ", "expected $:", NULL);
  if (val) {
    kev_template_convert_plain(out, tmpl, env_var, funcs);
  } else {
    kev_template_convert_plain_no_out(tmpl);
  }
  ch = kev_template_next_non_blank(tmpl);
  if (ch != '$')
    kev_throw_error("template: ", "expected $; or expected $|", NULL);
  if ((ch = fgetc(tmpl)) == '|') {
    if (val) {
      kev_template_convert_plain_no_out(tmpl);
    } else {
      kev_template_convert_plain(out, tmpl, env_var, funcs);
    }
    if (fgetc(tmpl) != '$' || fgetc(tmpl) != ';')
      kev_throw_error("template: ", "expected $;", NULL);
  }
  else if (ch != ';') {
    kev_throw_error("template: ", "expected $;", NULL);
  }
}

static void kev_template_condition_no_out(FILE* tmpl) {
  kev_template_expr_or(tmpl, NULL, NULL);
  int ch = kev_template_next_non_blank(tmpl);
  if (ch != '$' || fgetc(tmpl) != ':')
    kev_throw_error("template: ", "expected $:", NULL);
  kev_template_convert_plain_no_out(tmpl);
  ch = kev_template_next_non_blank(tmpl);
  if (ch != '$')
    kev_throw_error("template: ", "expected $; or expected $|", NULL);
  if ((ch = fgetc(tmpl)) == '|') {
      kev_template_convert_plain_no_out(tmpl);
    if (fgetc(tmpl) != '$' || fgetc(tmpl) != ';')
      kev_throw_error("template: ", "expected $;", NULL);
  }
  else if (ch != ';') {
    kev_throw_error("template: ", "expected $;", NULL);
  }
}

static bool kev_template_expr_or(FILE* tmpl, KevStringMap* env_var, KevFuncMap* funcs) {
  bool value = false;
  int ch = '|';
  while (ch == '|') {
    value = kev_template_expr_and(tmpl, env_var, funcs) || value;
    ch = kev_template_next_non_blank(tmpl);
  }
  ungetc(ch, tmpl);
  return value;
}

static bool kev_template_expr_and(FILE* tmpl, KevStringMap* env_var, KevFuncMap* funcs) {
  bool value = true;
  int ch = '&';
  while (ch == '&') {
    value = kev_template_expr_unit(tmpl, env_var, funcs) && value;
    ch = kev_template_next_non_blank(tmpl);
  }
  ungetc(ch, tmpl);
  return value;
}

static bool kev_template_expr_unit(FILE* tmpl, KevStringMap* env_var, KevFuncMap* funcs) {
  int ch = kev_template_next_non_blank(tmpl);
  if (ch == '!') {
    return !kev_template_expr_unit(tmpl, env_var, funcs);
  } else if (ch == '(') {
    bool value = kev_template_expr_or(tmpl, env_var, funcs);
    if (fgetc(tmpl) != ')')
      kev_throw_error("template: ", "missing \')\'", NULL);
    return value;
  } else {
    char buffer[1024];
    ungetc(ch, tmpl);
    ch = kev_template_read_id(tmpl, buffer, 1024);
    ungetc(ch, tmpl);
    if (!env_var && !funcs) return false;
    return (env_var && kev_strmap_search(env_var, buffer) != NULL) ||
           (funcs && kev_funcmap_search(funcs, buffer) != NULL);
  }
}

static int kev_template_next_non_blank(FILE* tmpl) {
  int ch = 0;
  while ((ch = fgetc(tmpl)) == ' ' || ch == '\t' || ch == '\n')
    continue;
  return ch;
}

static void kev_throw_error(const char* header, const char* info, const char* additional_info) {
  fprintf(stderr, "%s %s", header, info);
  if (additional_info)
    fputs(additional_info, stderr);
  fputc('\n', stderr);
  exit(EXIT_FAILURE);
}
