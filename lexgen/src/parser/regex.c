#include "lexgen/include/parser/regex.h"
#include "utils/include/string/kev_string.h"

#include <stdlib.h>

static uint64_t error_type = KEV_REGEX_ERR_NONE;
static const char* error_info = NULL;
static size_t error_pos = 0;

typedef struct tagKevParser {
  const uint8_t* regex;
  const uint8_t* current;
} KevParser;

static inline void kev_regex_next_char(KevParser* parser) {
  while (*++parser->current == ' ')
    continue;
}

static inline void kev_regex_clear_blank(KevParser* parser) {
  while (*parser->current == ' ') {
    ++parser->current;
  }
}

static KevFA* kev_regex(KevParser* parser, KevStringFaMap* nfa_map);
static KevFA* kev_regex_alternation(KevParser* parser, KevStringFaMap* nfa_map);
static KevFA* kev_regex_concatenation(KevParser* parser, KevStringFaMap* nfa_map);
static KevFA* kev_regex_unit(KevParser* parser, KevStringFaMap* nfa_map);
static bool kev_regex_post(KevParser* parser, KevFA* nfa);
static bool kev_regex_post_repeat(KevParser* parser, KevFA* nfa);
static KevFA* kev_regex_charset(KevParser* parser, KevStringFaMap* nfa_map);
static int kev_regex_escape(KevParser* parser);
static KevFA* kev_regex_escape_nfa(KevParser* parser);
/* do concatenation bnetween 'dest' and 'src', but do not modify 'src' */
static bool kev_nfa_append(KevFA* dest, KevFA* src);
static bool kev_char_range(KevFA* nfa, int64_t begin, int64_t end);
uint8_t* kev_regex_ref_name(KevParser* parser);
KevFA* kev_get_named_nfa(uint8_t* name, KevStringFaMap* nfa_map);
static void kev_regex_set_error_info(const char* info);

KevFA* kev_regex_parse(const uint8_t* regex, KevStringFaMap* nfa_map) {
  error_type = KEV_REGEX_ERR_NONE;
  if (!regex) {
    error_type = KEV_REGEX_ERR_INVALID_INPUT;
    kev_regex_set_error_info("empty input");
    return NULL;
  }
  KevParser parser;
  parser.regex = regex;
  parser.current = regex;
  kev_regex_clear_blank(&parser);
  KevFA* nfa = kev_regex(&parser, nfa_map);
  if (!nfa) {
    error_pos = parser.current - parser.regex;
    return NULL;
  }
  if (*parser.current != '\0') {
    error_type = KEV_REGEX_ERR_SYNTAX;
    kev_regex_set_error_info("expected \'\\0\'");
    kev_fa_delete(nfa);
    error_pos = parser.current - parser.regex;
    return NULL;
  }
  return nfa;
}

static KevFA* kev_regex(KevParser* parser, KevStringFaMap* nfa_map) {
  KevFA* nfa = kev_regex_alternation(parser, nfa_map);
  if (!nfa) return NULL;
  if (*parser->current != '\0') {
    error_type = KEV_REGEX_ERR_SYNTAX;
    kev_regex_set_error_info("expected end of the expression");
    kev_fa_delete(nfa);
    return NULL;
  }
  return nfa;
}

static KevFA* kev_regex_alternation(KevParser* parser, KevStringFaMap* nfa_map) {
  KevFA* nfa = kev_regex_concatenation(parser, nfa_map);
  if (!nfa) return NULL;
  while (*parser->current == '|') {
    kev_regex_next_char(parser);
    KevFA* nfa1 = kev_regex_concatenation(parser, nfa_map);
    if (!nfa1) {
      kev_fa_delete(nfa);
      return NULL;
    }
    if (!kev_nfa_alternation(nfa, nfa1)) {
      error_type = KEV_REGEX_ERR_GENERATE;
      kev_regex_set_error_info("failed to do alternation");
      kev_fa_delete(nfa);
      kev_fa_delete(nfa1);
      return NULL;
    }
    kev_fa_delete(nfa1);
  }
  return nfa;
}

static KevFA* kev_regex_concatenation(KevParser* parser, KevStringFaMap* nfa_map) {
  KevFA* nfa = kev_regex_unit(parser, nfa_map);
  if (!nfa) return NULL;
  uint8_t ch = *parser->current;
  while (ch != '|' && ch != '\0' && ch != ')') {
    KevFA* nfa1 = kev_regex_unit(parser, nfa_map);
    if (!nfa1) {
      kev_fa_delete(nfa);
      return NULL;
    }
    if (!kev_nfa_concatenation(nfa, nfa1)) {
      error_type = KEV_REGEX_ERR_GENERATE;
      kev_regex_set_error_info("failed to do concatenation");
      kev_fa_delete(nfa);
      kev_fa_delete(nfa1);
      return NULL;
    }
    kev_fa_delete(nfa1);
    ch = *parser->current;
  }
  return nfa;
}


static KevFA* kev_regex_unit(KevParser* parser, KevStringFaMap* nfa_map) {
  uint8_t ch = *parser->current;
  KevFA* nfa = NULL;
  switch (ch) {
    case '(': {
      kev_regex_next_char(parser);
      nfa = kev_regex_alternation(parser, nfa_map);
      if (!nfa) return NULL;
      if (*parser->current != ')') {
        error_type = KEV_REGEX_ERR_SYNTAX;
        kev_regex_set_error_info("expected \')\'");
        kev_fa_delete(nfa);
        return NULL;
      }
      kev_regex_next_char(parser);
      break;
    }
    case '[': {
      nfa = kev_regex_charset(parser, nfa_map);
      if (!nfa) return NULL;
      break;
    }
    case '.': {
      nfa = kev_nfa_create(ch);
      if (!nfa ||
          !kev_char_range(nfa, 0, 256)) {
        kev_fa_delete(nfa);
        error_type = KEV_REGEX_ERR_GENERATE;
        kev_regex_set_error_info("failed to create NFA");
        return NULL;
      }
      kev_regex_next_char(parser);
      break;
    }
    case '\\': {
      nfa = kev_regex_escape_nfa(parser);
      if (!nfa) return NULL;
      break;
    }
    case '{':
    case '}':
    case ']':
    case '|':
    case '+':
    case '?':
    case '*':
    case '\0':
    case ')': {
      error_type = KEV_REGEX_ERR_SYNTAX;
      kev_regex_set_error_info("expected character");
      return NULL;
      break;
    }
    default: {
      nfa = kev_nfa_create(ch);
      if (!nfa) {
        error_type = KEV_REGEX_ERR_GENERATE;
        kev_regex_set_error_info("failed to create NFA");
        return NULL;
      }
      kev_regex_next_char(parser);
      break;
    }
  }
  kev_regex_post(parser, nfa);
  return nfa;
}

static int kev_regex_escape(KevParser* parser) {
  uint8_t ch = *++parser->current;
  int64_t number = 0;
  switch (ch) {
    case '\0': {
      error_type = KEV_REGEX_ERR_SYNTAX;
      kev_regex_set_error_info("unexpected \'\\0\'");
      number = -1;
      break;
    }
    case '0': {
      char* pos = NULL;
      number = strtoull((char*)parser->current, &pos, 8);
      parser->current = (uint8_t*)pos;
      break;
    }
    case 'u':
    case 'x': {
      char* pos = NULL;
      number = strtoull((char*)parser->current + 1, &pos, 16);
      parser->current = (uint8_t*)pos;
      break;
    }
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9': {
      char* pos = NULL;
      number = strtoull((char*)parser->current, &pos, 10);
      parser->current = (uint8_t*)pos;
      break;
    }
    case 'n': {
      number = '\n';
      parser->current++;
      break;
    }
    case 'r': {
      number = '\r';
      parser->current++;
      break;
    }
    case 't': {
      number = '\t';
      parser->current++;
      break;
    }
    case 'a': {
      number = '\a';
      parser->current++;
      break;
    }
    default: {
      number = ch;
      parser->current++;
      break;
    }
  }
  kev_regex_clear_blank(parser);
  return number;
}

static inline KevFA* kev_regex_escape_nfa(KevParser* parser) {
  int number = kev_regex_escape(parser);
  if (number < 0) return NULL;
  KevFA* nfa = kev_nfa_create(number);
  if (!nfa) {
    error_type = KEV_REGEX_ERR_GENERATE;
    kev_regex_set_error_info("failed to create NFA");
    return NULL;
  }
  return nfa;
}

static void kev_regex_set_error_info(const char* info) {
  error_info = info;
}

static bool kev_regex_post(KevParser* parser, KevFA* nfa) {
  /* *+{m, n}? */
  while (true) {
    switch (*parser->current) {
      case '*': {
        if (!kev_nfa_kleene(nfa)) {
          kev_fa_delete(nfa);
          error_type = KEV_REGEX_ERR_GENERATE;
          kev_regex_set_error_info("failed to do kleene closure");
          return false;
        }
        kev_regex_next_char(parser);
        break;
      }
      case '+': {
        if (!kev_nfa_positive(nfa)) {
          kev_fa_delete(nfa);
          error_type = KEV_REGEX_ERR_GENERATE;
          kev_regex_set_error_info("failed to do positive closure");
          return false;
        }
        kev_regex_next_char(parser);
        break;
      }
      case '?': {
        if (!kev_nfa_add_epsilon(nfa)) {
          kev_fa_delete(nfa);
          error_type = KEV_REGEX_ERR_GENERATE;
          kev_regex_set_error_info("failed to do ? operation");
          return false;
        }
        kev_regex_next_char(parser);
        break;
      }
      case '{': {
        if (!kev_regex_post_repeat(parser, nfa)) {
          return false;
        }
        break;
      }
      default: {
        return true;
      }
    }
  }
}

static bool kev_regex_post_repeat(KevParser* parser, KevFA* nfa) {
  kev_regex_next_char(parser);
  size_t m = 0;
  size_t n = 0;
  if (*parser->current <= '9' && *parser->current >= '0') {
    uint8_t* pos = NULL;
    m = strtoull((char*)parser->current, (char**)&pos, 10);
    parser->current = pos;
  } else {
    kev_fa_delete(nfa);
    error_type = KEV_REGEX_ERR_SYNTAX;
    kev_regex_set_error_info("expected decimal integer");
    return false;
  }
  kev_regex_clear_blank(parser);
  if (*parser->current == '}') {
    n = m;
    kev_regex_next_char(parser);
  } else if (*parser->current == ',') {
    kev_regex_next_char(parser);
    if (*parser->current <= '9' && *parser->current >= '0') {
      uint8_t* pos = NULL;
      n = strtoull((char*)parser->current, (char**)&pos, 10);
      parser->current = pos;
    } else {
      kev_fa_delete(nfa);
      error_type = KEV_REGEX_ERR_SYNTAX;
      kev_regex_set_error_info("expected decimal integer");
      return false;
    }
    kev_regex_clear_blank(parser);
    if (*parser->current != '}') {
      kev_fa_delete(nfa);
      error_type = KEV_REGEX_ERR_SYNTAX;
      kev_regex_set_error_info("expected \'}\'");
      return false;
    }
    kev_regex_next_char(parser);
  } else {
    kev_fa_delete(nfa);
    error_type = KEV_REGEX_ERR_SYNTAX;
    kev_regex_set_error_info("expected \'}\' or \',\'");
    return false;
  }

  if (m > n) {
    kev_fa_delete(nfa);
    error_type = KEV_REGEX_ERR_SYNTAX;
    kev_regex_set_error_info("left number should not be larger than right number");
    return false;
  }

  KevFA result;
  if (!kev_nfa_init_epsilon(&result)) {
    kev_fa_delete(nfa);
    error_type = KEV_REGEX_ERR_GENERATE;
    kev_regex_set_error_info("failed to repeat");
    return false;
  }
  
  for (size_t i = 0; i < m; ++i) {
    if (!kev_nfa_append(&result, nfa)) {
      kev_fa_destroy(&result);
      kev_fa_delete(nfa);
      error_type = KEV_REGEX_ERR_GENERATE;
      kev_regex_set_error_info("failed to repeat");
      return false;
    }
  }
  if (!kev_nfa_add_epsilon(nfa)) {
      kev_fa_destroy(&result);
      kev_fa_delete(nfa);
      error_type = KEV_REGEX_ERR_GENERATE;
      kev_regex_set_error_info("failed to repeat");
      return false;
  }
  for (size_t i = m; i < n; ++i) {
    if (!kev_nfa_append(&result, nfa)) {
      kev_fa_destroy(&result);
      kev_fa_delete(nfa);
      error_type = KEV_REGEX_ERR_GENERATE;
      kev_regex_set_error_info("failed to repeat");
      return false;
    }
  }
  kev_fa_destroy(nfa);
  kev_fa_init_move(nfa, &result);
  return true;
}

static KevFA* kev_regex_charset(KevParser* parser, KevStringFaMap* nfa_map) {
  bool in_charset[256];
  bool mark = true;
  kev_regex_next_char(parser);
  if (*parser->current == '^') {
    mark = false;
    kev_regex_next_char(parser);
  } else if (*parser->current == ':') {
    kev_regex_next_char(parser);
    uint8_t* name = kev_regex_ref_name(parser);
    if (!name) return NULL;
    if (*parser->current != ':') {
      free(name);
      error_type = KEV_REGEX_ERR_SYNTAX;
      kev_regex_set_error_info("expected \':\' after charset name");
      return NULL;
    }
    kev_regex_next_char(parser);
    if (*parser->current != ']') {
      free(name);
      error_type = KEV_REGEX_ERR_SYNTAX;
      kev_regex_set_error_info("expected \']\'");
      return NULL;
    }
    kev_regex_next_char(parser);
    KevFA* nfa = kev_get_named_nfa(name, nfa_map);
    free(name);
    return nfa;
  }
  for (int i = 0; i < 256; ++i)
    in_charset[i] = !mark;
  while (true) {
    uint8_t begin = 0;
    uint8_t end = 0;
    if (*parser->current == ']') {
      kev_regex_next_char(parser);
      break;
    } else if (*parser->current == '\\') {
      int number = kev_regex_escape(parser);
      if (number < 0) return NULL;
      begin = (uint8_t)number;
    } else if (*parser->current == '\0' || *parser->current == '-') {
      error_type = KEV_REGEX_ERR_SYNTAX;
      kev_regex_set_error_info("expected \']\'");
      return NULL;
    } else {
      begin = *parser->current;
      kev_regex_next_char(parser);
    }
    if (*parser->current == '-') {
      kev_regex_next_char(parser);
      if (*parser->current == '\\') {
        int number = kev_regex_escape(parser);
        if (number < 0) return NULL;
        end = (uint8_t)number;
      } else if (*parser->current == ']' || *parser->current == '\0' || *parser->current == '-') {
        error_type = KEV_REGEX_ERR_SYNTAX;
        kev_regex_set_error_info("expected character");
        return NULL;
      } else {
        end = *parser->current;
        kev_regex_next_char(parser);
      }
    } else {
      end = begin;
    }
    for (int i = begin; i <= end; ++i)
      in_charset[i] = mark;
  }
  KevFA* nfa = kev_nfa_create_empty();
  if (!nfa) {
    kev_fa_delete(nfa);
    error_type = KEV_REGEX_ERR_GENERATE;
    kev_regex_set_error_info("failed to create charset");
  }
  for (int i = 0; i < 256; ++i) {
    if (in_charset[i] && !kev_nfa_add_transition(nfa, i)) {
      kev_fa_delete(nfa);
      error_type = KEV_REGEX_ERR_GENERATE;
      kev_regex_set_error_info("failed to create charset");
    }
  }
  return nfa;
}

static bool kev_nfa_append(KevFA* dest, KevFA* src) {
  KevFA src_copy;
  if (!kev_fa_init_copy(&src_copy, src)) {
    return false;
  }
  if (!kev_nfa_concatenation(dest, &src_copy)) {
    kev_fa_destroy(&src_copy);
    return false;
  }
  return true;
}

static inline bool kev_char_range(KevFA* nfa, int64_t begin, int64_t end) {
  if (begin < 0 || end < 0) return false;
  for (int64_t c = begin; c < end; ++c) {
    if (!kev_nfa_add_transition(nfa, c)) {
      return false;
    }
  }
  return true;
}

uint8_t* kev_regex_ref_name(KevParser* parser) {
  const uint8_t* name_end = parser->current;
  if (((*name_end | 0x20) <= 'z' && (*name_end | 0x20) >= 'a') ||
      *name_end == '_' || *name_end == '-')
    ++name_end;
  while (((*name_end | 0x20) <= 'z' && (*name_end | 0x20) >= 'a') ||
         *name_end == '_' || (*name_end <= '9' && *name_end >= '0') || *name_end == '-') {
    ++name_end;
  }
  if (name_end == parser->current) {
    error_type = KEV_REGEX_ERR_SYNTAX;
    kev_regex_set_error_info("identifier can not be empty string");
    return NULL;
  }
  uint8_t* name = (uint8_t*)kev_str_copy_len((char*)parser->current, name_end - parser->current);
  if (!name) {
    error_type = KEV_REGEX_ERR_GENERATE;
    kev_regex_set_error_info("out of memory");
    return NULL;
  }
  parser->current = name_end;
  kev_regex_clear_blank(parser);
  return name;
}

KevFA* kev_get_named_nfa(uint8_t* name, KevStringFaMap* nfa_map) {
  char* nfa_name = (char*)name;
  if (!nfa_map) {
    error_type = KEV_REGEX_ERR_SYNTAX;
    kev_regex_set_error_info("no such NFA");
    return NULL;
  }
  KevStringFaMapNode* itr = kev_strfamap_search(nfa_map, nfa_name);
  if (itr) {
    KevFA* nfa = kev_fa_create_copy(itr->value);
    if (!nfa) {
      error_type = KEV_REGEX_ERR_GENERATE;
      kev_regex_set_error_info("failed to copy NFA in kev_get_named_nfa");
      return NULL;
    }
    return nfa;
  } else {
    error_type = KEV_REGEX_ERR_SYNTAX;
    kev_regex_set_error_info("unknown NFA name");
    return NULL;
  }
} 

uint64_t kev_regex_get_error(void) {
  return error_type;
}

const char* kev_regex_get_info(void) {
  return error_info;
}

size_t kev_regex_get_pos(void) {
  return error_pos;
}
