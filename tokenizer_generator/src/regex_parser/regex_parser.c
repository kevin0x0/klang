#include "tokenizer_generator/include/regex_parser/regex_parser.h"
#include "tokenizer_generator/include/regex_parser/hashmap/strfa_map.h"
#include "tokenizer_generator/include/finite_automaton/finite_automaton.h"

#include <stdlib.h>
#include <string.h>


static uint64_t parser_error = KEV_REGEX_ERR_NONE;
static char* error_info = NULL;

static KevStringFaMap* named_nfa = NULL;

typedef struct tagKevParser {
  uint8_t* regex;
  uint8_t* current;
} KevParser;

static inline void kev_next_char(KevParser* parser) {
  while (*++parser->current == ' ')
    continue;
}

static inline void kev_parser_clear_blank(KevParser* parser) {
  while (*parser->current == ' ') {
    ++parser->current;
  }
}

static KevFA* kev_regex(KevParser* parser);
static KevFA* kev_regex_alternation(KevParser* parser);
static KevFA* kev_regex_concatenation(KevParser* parser);
static KevFA* kev_regex_unit(KevParser* parser);
static bool kev_regex_post(KevParser* parser, KevFA* nfa);
static bool kev_regex_post_repeat(KevParser* parser, KevFA* nfa);
static KevFA* kev_regex_charset(KevParser* parser);
static int kev_regex_escape(KevParser* parser);
static KevFA* kev_regex_escape_nfa(KevParser* parser);
/* do not modify 'src' */
static bool kev_nfa_append(KevFA* dest, KevFA* src);
static bool kev_char_range(KevFA* nfa, int64_t begin, int64_t end);

uint8_t* kev_regex_ref_name(KevParser* parser);
KevFA* kev_get_named_nfa(uint8_t* name);

static void kev_regex_set_error_info(char* info);



KevFA* kev_regex_parse(uint8_t* regex) {
  parser_error = KEV_REGEX_ERR_NONE;
  if (!regex) {
    parser_error = KEV_REGEX_ERR_INVALID_INPUT;
    kev_regex_set_error_info("empty input");
    return NULL;
  }
  KevParser parser;
  parser.regex = regex;
  parser.current = regex;
  kev_parser_clear_blank(&parser);
  KevFA* nfa = kev_regex(&parser);
  if (!nfa) return nfa;
  if (*parser.current != '\0') {
    parser_error = KEV_REGEX_ERR_SYNTAX;
    kev_regex_set_error_info("expected \'\\0\'");
    kev_fa_delete(nfa);
    return NULL;
  }
  return nfa;
}

static KevFA* kev_regex(KevParser* parser) {
  KevFA* nfa = kev_regex_alternation(parser);
  if (!nfa) return NULL;
  if (*parser->current != '\0') {
    parser_error = KEV_REGEX_ERR_SYNTAX;
    kev_regex_set_error_info("expected end of the expression");
    kev_fa_delete(nfa);
    return NULL;
  }
  return nfa;
}

static KevFA* kev_regex_alternation(KevParser* parser) {
  KevFA* nfa = kev_regex_concatenation(parser);
  if (!nfa) return NULL;
  while (*parser->current == '|') {
    kev_next_char(parser);
    KevFA* nfa1 = kev_regex_concatenation(parser);
    if (!nfa1) {
      kev_fa_delete(nfa);
      return NULL;
    }
    if (!kev_nfa_alternation(nfa, nfa1)) {
      parser_error = KEV_REGEX_ERR_GENERATE;
      kev_regex_set_error_info("failed to do alternation");
      kev_fa_delete(nfa);
      kev_fa_delete(nfa1);
      return NULL;
    }
    kev_fa_delete(nfa1);
  }
  return nfa;
}

static KevFA* kev_regex_concatenation(KevParser* parser) {
  KevFA* nfa = kev_regex_unit(parser);
  if (!nfa) return NULL;
  uint8_t ch = *parser->current;
  while (ch != '|' && ch != '\0' && ch != ')') {
    KevFA* nfa1 = kev_regex_unit(parser);
    if (!nfa1) {
      kev_fa_delete(nfa);
      return NULL;
    }
    if (!kev_nfa_concatenation(nfa, nfa1)) {
      parser_error = KEV_REGEX_ERR_GENERATE;
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


static KevFA* kev_regex_unit(KevParser* parser) {
  static uint8_t illegal[256] = {
    ['{'] = 1, ['}'] = 1, [')'] = 1, [']'] = 1,
    ['|'] = 1, ['+'] = 1, ['?'] = 1, ['*'] = 1,
    ['\0'] = 1
  };
  uint8_t ch = *parser->current;
  KevFA* nfa = NULL;
  if (ch == '(') {
    kev_next_char(parser);
    nfa = kev_regex_alternation(parser);
    if (!nfa) return NULL;
    if (*parser->current != ')') {
      parser_error = KEV_REGEX_ERR_SYNTAX;
      kev_regex_set_error_info("expected \')\'");
      kev_fa_delete(nfa);
      return NULL;
    }
    kev_next_char(parser);
  } else if (ch == '[') {
    nfa = kev_regex_charset(parser);
    if (!nfa) return NULL;
  } else if (ch == '.') {
    nfa = kev_nfa_create(ch);
    if (!nfa ||
        !kev_char_range(nfa, 0, 256)) {
      kev_fa_delete(nfa);
      parser_error = KEV_REGEX_ERR_GENERATE;
      kev_regex_set_error_info("failed to create NFA");
      return NULL;
    }
    kev_next_char(parser);
  } else if (illegal[ch]) {
    parser_error = KEV_REGEX_ERR_SYNTAX;
    kev_regex_set_error_info("expected character");
    return NULL;
  } else if (ch == '\\') {
    nfa = kev_regex_escape_nfa(parser);
    if (!nfa) return NULL;
  } else {
    nfa = kev_nfa_create(ch);
    if (!nfa) {
      parser_error = KEV_REGEX_ERR_GENERATE;
      kev_regex_set_error_info("failed to create NFA");
      return NULL;
    }
    kev_next_char(parser);
  }
  kev_regex_post(parser, nfa);
  return nfa;
}

static int kev_regex_escape(KevParser* parser) {
  uint8_t ch = *++parser->current;
  int64_t number = 0;
  switch (ch) {
    case '\0': {
      parser_error = KEV_REGEX_ERR_SYNTAX;
      kev_regex_set_error_info("unexpected \'\\0\'");
      number = -1;
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
  kev_parser_clear_blank(parser);
  return number;
}

static inline KevFA* kev_regex_escape_nfa(KevParser* parser) {
  int number = kev_regex_escape(parser);
  if (number < 0) return NULL;
  KevFA* nfa = kev_nfa_create(number);
  if (!nfa) {
    parser_error = KEV_REGEX_ERR_GENERATE;
    kev_regex_set_error_info("failed to create NFA");
    return NULL;
  }
  return nfa;
}

static void kev_regex_set_error_info(char* info) {
  error_info = info;
}

static bool kev_regex_post(KevParser* parser, KevFA* nfa) {
  /* *+{m, n}? */
  while (true) {
    switch (*parser->current) {
      case '*': {
        if (!kev_nfa_kleene(nfa)) {
          kev_fa_delete(nfa);
          parser_error = KEV_REGEX_ERR_GENERATE;
          kev_regex_set_error_info("failed to do kleene closure");
          return false;
        }
        kev_next_char(parser);
        break;
      }
      case '+': {
        if (!kev_nfa_positive(nfa)) {
          kev_fa_delete(nfa);
          parser_error = KEV_REGEX_ERR_GENERATE;
          kev_regex_set_error_info("failed to do positive closure");
          return false;
        }
        kev_next_char(parser);
        break;
      }
      case '?': {
        if (!kev_nfa_add_transition(nfa, KEV_NFA_SYMBOL_EPSILON)) {
          kev_fa_delete(nfa);
          parser_error = KEV_REGEX_ERR_GENERATE;
          kev_regex_set_error_info("failed to do ? operation");
          return false;
        }
        kev_next_char(parser);
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
  kev_next_char(parser);
  uint64_t m = 0;
  uint64_t n = 0;
  if (*parser->current <= '9' && *parser->current >= '0') {
    uint8_t* pos = NULL;
    m = strtoull((char*)parser->current, (char**)&pos, 10);
    parser->current = pos;
  } else {
    kev_fa_delete(nfa);
    parser_error = KEV_REGEX_ERR_SYNTAX;
    kev_regex_set_error_info("expected decimal integer");
    return false;
  }
  kev_parser_clear_blank(parser);
  if (*parser->current == '}') {
    n = m;
    kev_next_char(parser);
  } else if (*parser->current == ',') {
    kev_next_char(parser);
    if (*parser->current <= '9' && *parser->current >= '0') {
      uint8_t* pos = NULL;
      n = strtoull((char*)parser->current, (char**)&pos, 10);
      parser->current = pos;
    } else {
      kev_fa_delete(nfa);
      parser_error = KEV_REGEX_ERR_SYNTAX;
      kev_regex_set_error_info("expected decimal integer");
      return false;
    }
    kev_parser_clear_blank(parser);
    if (*parser->current != '}') {
      kev_fa_delete(nfa);
      parser_error = KEV_REGEX_ERR_SYNTAX;
      kev_regex_set_error_info("expected \'}\'");
      return false;
    }
    kev_next_char(parser);
  } else {
    kev_fa_delete(nfa);
    parser_error = KEV_REGEX_ERR_SYNTAX;
    kev_regex_set_error_info("expected \'}\' or \',\'");
    return false;
  }

  if (m > n) {
    kev_fa_delete(nfa);
    parser_error = KEV_REGEX_ERR_SYNTAX;
    kev_regex_set_error_info("left number should not be larger than right number");
    return false;
  }

  KevFA result;
  if (!kev_nfa_init(&result, KEV_NFA_SYMBOL_EPSILON)) {
    kev_fa_delete(nfa);
    parser_error = KEV_REGEX_ERR_GENERATE;
    kev_regex_set_error_info("failed to repeat");
    return false;
  }
  
  for (uint64_t i = 0; i < m; ++i) {
    if (!kev_nfa_append(&result, nfa)) {
      kev_fa_destroy(&result);
      kev_fa_delete(nfa);
      parser_error = KEV_REGEX_ERR_GENERATE;
      kev_regex_set_error_info("failed to repeat");
      return false;
    }
  }
  if (!kev_nfa_add_transition(nfa, KEV_NFA_SYMBOL_EPSILON)) {
      kev_fa_destroy(&result);
      kev_fa_delete(nfa);
      parser_error = KEV_REGEX_ERR_GENERATE;
      kev_regex_set_error_info("failed to repeat");
      return false;
  }
  for (uint64_t i = m; i < n; ++i) {
    if (!kev_nfa_append(&result, nfa)) {
      kev_fa_destroy(&result);
      kev_fa_delete(nfa);
      parser_error = KEV_REGEX_ERR_GENERATE;
      kev_regex_set_error_info("failed to repeat");
      return false;
    }
  }
  kev_fa_destroy(nfa);
  kev_fa_init_move(nfa, &result);
  return true;
}

static KevFA* kev_regex_charset(KevParser* parser) {
  bool in_charset[256];
  bool mark = true;
  kev_next_char(parser);
  if (*parser->current == '^') {
    mark = false;
    kev_next_char(parser);
  } else if (*parser->current == ':') {
    kev_next_char(parser);
    uint8_t* name = kev_regex_ref_name(parser);
    if (!name) return NULL;
    if (*parser->current != ':') {
      free(name);
      parser_error = KEV_REGEX_ERR_SYNTAX;
      kev_regex_set_error_info("expected \':\'");
      return NULL;
    }
    kev_next_char(parser);
    if (*parser->current != ']') {
      free(name);
      parser_error = KEV_REGEX_ERR_SYNTAX;
      kev_regex_set_error_info("expected \']\'");
      return NULL;
    }
    kev_next_char(parser);
    KevFA* nfa = kev_get_named_nfa(name);
    free(name);
    return nfa;
  }
  for (int i = 0; i < 256; ++i)
    in_charset[i] = !mark;
  while (true) {
    uint8_t begin = 0;
    uint8_t end = 0;
    if (*parser->current == ']') {
      kev_next_char(parser);
      break;
    } else if (*parser->current == '\\') {
      int number = kev_regex_escape(parser);
      if (number < 0) return NULL;
      begin = (uint8_t)number;
    } else if (*parser->current == '\0' || *parser->current == '-') {
      parser_error = KEV_REGEX_ERR_SYNTAX;
      kev_regex_set_error_info("expected \']\'");
      return NULL;
    } else {
      begin = *parser->current;
      kev_next_char(parser);
    }
    if (*parser->current == '-') {
      kev_next_char(parser);
      if (*parser->current == '\\') {
        int number = kev_regex_escape(parser);
        if (number < 0) return NULL;
        end = (uint8_t)number;
      } else if (*parser->current == ']' || *parser->current == '\0' || *parser->current == '-') {
        parser_error = KEV_REGEX_ERR_SYNTAX;
        kev_regex_set_error_info("expected character");
        return NULL;
      } else {
        end = *parser->current;
        kev_next_char(parser);
      }
    } else {
      end = begin;
    }
    for (int i = begin; i <= end; ++i)
      in_charset[i] = mark;
  }
  KevFA* nfa = kev_nfa_create(KEV_NFA_SYMBOL_EMPTY);
  if (!nfa) {
    kev_fa_delete(nfa);
    parser_error = KEV_REGEX_ERR_GENERATE;
    kev_regex_set_error_info("failed to create charset");
  }
  for (int i = 0; i < 256; ++i) {
    if (in_charset[i] && !kev_nfa_add_transition(nfa, i)) {
      kev_fa_delete(nfa);
      parser_error = KEV_REGEX_ERR_GENERATE;
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
  uint8_t* name_end = parser->current;
  while ((*name_end <= 'z' && *name_end >= 'a') ||
         (*name_end <= 'Z' && *name_end >= 'A')) {
    ++name_end;
  }
  uint8_t* name = (uint8_t*)malloc(sizeof (uint8_t) * (name_end - parser->current + 1));
  if (!name) {
    parser_error = KEV_REGEX_ERR_GENERATE;
    kev_regex_set_error_info("out of memory");
    return NULL;
  }
  memcpy(name, parser->current, (name_end - parser->current) * sizeof (uint8_t));
  name[name_end - parser->current] = '\0';
  parser->current = name_end;
  kev_parser_clear_blank(parser);
  return name;
}

KevFA* kev_get_named_nfa(uint8_t* name) {
  char* nfa_name = (char*)name;
  KevFA* nfa = kev_nfa_create(KEV_NFA_SYMBOL_EMPTY);
  if (!nfa) {
    parser_error = KEV_REGEX_ERR_GENERATE;
    kev_regex_set_error_info("failed to create nfa in kev_get_named_nfa");
    return NULL;
  }
  if (strcmp(nfa_name, "print") == 0) {
    if (!kev_char_range(nfa, 32, 127)) {
      kev_fa_delete(nfa);
      parser_error = KEV_REGEX_ERR_GENERATE;
      kev_regex_set_error_info("failed to create charset \'print\' in kev_get_named_nfa");
      return NULL;
    }
  } else if (strcmp(nfa_name, "graph") == 0) {
    if (!kev_char_range(nfa, 33, 127)) {
      kev_fa_delete(nfa);
      parser_error = KEV_REGEX_ERR_GENERATE;
      kev_regex_set_error_info("failed to create charset \'graph\' in kev_get_named_nfa");
      return NULL;
    }
  } else if (strcmp(nfa_name, "alnum") == 0) {
    if (!kev_char_range(nfa, 'a', 'z' + 1) ||
        !kev_char_range(nfa, 'A', 'Z' + 1) ||
        !kev_char_range(nfa, '0', '9' + 1)) {
      kev_fa_delete(nfa);
      parser_error = KEV_REGEX_ERR_GENERATE;
      kev_regex_set_error_info("failed to create charset \'alnum\' in kev_get_named_nfa");
      return NULL;
    }
  } else if (strcmp(nfa_name, "alpha") == 0) {
    if (!kev_char_range(nfa, 'a', 'z' + 1) ||
        !kev_char_range(nfa, 'A', 'Z' + 1)) {
      kev_fa_delete(nfa);
      parser_error = KEV_REGEX_ERR_GENERATE;
      kev_regex_set_error_info("failed to create charset \'alpha\' in kev_get_named_nfa");
      return NULL;
    }
  } else if (strcmp(nfa_name, "digit") == 0) {
    if (!kev_char_range(nfa, '0', '9' + 1)) {
      kev_fa_delete(nfa);
      parser_error = KEV_REGEX_ERR_GENERATE;
      kev_regex_set_error_info("failed to create charset \'digit\' in kev_get_named_nfa");
      return NULL;
    }
  } else if (strcmp(nfa_name, "lower") == 0) {
    if (!kev_char_range(nfa, 'a', 'z' + 1)) {
      kev_fa_delete(nfa);
      parser_error = KEV_REGEX_ERR_GENERATE;
      kev_regex_set_error_info("failed to create charset \'lower\' in kev_get_named_nfa");
      return NULL;
    }
  } else if (strcmp(nfa_name, "upper") == 0) {
    if (!kev_char_range(nfa, 'A', 'Z' + 1)) {
      kev_fa_delete(nfa);
      parser_error = KEV_REGEX_ERR_GENERATE;
      kev_regex_set_error_info("failed to create charset \'upper\' in kev_get_named_nfa");
      return NULL;
    }
  } else {
    if (!named_nfa && !(named_nfa = kev_strfamap_create(8))) {
      parser_error = KEV_REGEX_ERR_GENERATE;
      kev_regex_set_error_info("failed to initialize 'named_nfa' in kev_get_named_nfa");
      return NULL;
    }
    KevStringFaMapNode* itr = kev_strfamap_search(named_nfa, nfa_name);
    if (itr) {
      kev_fa_destroy(nfa);
      if (!kev_fa_init_copy(nfa, itr->value)) {
        kev_fa_delete(nfa);
        parser_error = KEV_REGEX_ERR_GENERATE;
        kev_regex_set_error_info("failed to copy NFA in kev_get_named_nfa");
        return NULL;
      }
    } else {
      kev_fa_delete(nfa);
      parser_error = KEV_REGEX_ERR_SYNTAX;
      kev_regex_set_error_info("unknown NFA name");
      return NULL;
    }
}
  return nfa;
} 

uint64_t kev_regex_get_error(void) {
  return parser_error;
}

char* kev_regex_get_info(void) {
  return error_info;
}

bool kev_regex_insert_named_nfa(char* name, KevFA* nfa) {
  if (!named_nfa && !(named_nfa = kev_strfamap_create(8)))
    return false;
  if (!kev_strfamap_insert(named_nfa, name, nfa))
    return false;
  return true;
}

void kev_regex_destroy_named_nfa(void) {
  kev_strfamap_delete(named_nfa);
  named_nfa = NULL;
}
