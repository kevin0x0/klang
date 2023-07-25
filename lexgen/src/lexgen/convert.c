#include "lexgen/include/lexgen/convert.h"
#include <stdlib.h>

static void kev_lexgen_convert_pattern_mapping(KevPatternBinary* binary_info, KevParserState* parser_state,
                                                    size_t* acc_mapping);
static void kev_lexgen_convert_callback_array(KevPatternBinary* binary_info,
                                                  KevParserState* parser_state, size_t* acc_mapping);
static void kev_lexgen_convert_generate(KevPatternBinary* binary_info, KevParserState* parser_state, KevFA** p_min_dfa,
                                        size_t** p_acc_mapping);
static void kev_lexgen_convert_infos(KevPatternBinary* binary_info, KevParserState* parser_state);
static void kev_lexgen_convert_macro_array(KevPatternBinary* binary_info, KevParserState* parser_state);
static void kev_lexgen_convert_table(KevPatternBinary* binary_info, KevParserState* parser_state, KevFA* dfa);

static uint8_t (*kev_lexgen_convert_table_256_u8(KevFA* dfa))[256];
static uint8_t (*kev_lexgen_convert_table_128_u8(KevFA* dfa))[128];
static uint16_t (*kev_lexgen_convert_table_256_u16(KevFA* dfa))[256];
static uint16_t (*kev_lexgen_convert_table_128_u16(KevFA* dfa))[128];

static void fatal_error(char* info, char* info2);

void kev_lexgen_convert(KevPatternBinary* binary_info, KevParserState* parser_state) {
  KevFA* dfa;
  size_t* acc_mapping;
  kev_lexgen_convert_generate(binary_info, parser_state, &dfa, &acc_mapping);
  kev_lexgen_convert_callback_array(binary_info, parser_state, acc_mapping);
  kev_lexgen_convert_pattern_mapping(binary_info, parser_state, acc_mapping);
  free(acc_mapping);
  kev_lexgen_convert_table(binary_info, parser_state, dfa);
  kev_fa_delete(dfa);
  kev_lexgen_convert_infos(binary_info, parser_state);
  kev_lexgen_convert_macro_array(binary_info, parser_state);
}

void kev_lexgen_convert_destroy(KevPatternBinary* binary_info) {
  if (!binary_info) return;
  free(binary_info->macros);
  free(binary_info->callbacks);
  free(binary_info->infos);
  free(binary_info->table);
  free(binary_info->pattern_mapping);
  binary_info->macros = NULL;
  binary_info->callbacks = NULL;
  binary_info->infos = NULL;
  binary_info->table = NULL;
  binary_info->pattern_mapping = NULL;
}

static void kev_lexgen_convert_callback_array(KevPatternBinary* binary_info,
                                                KevParserState* parser_state, size_t* acc_mapping) {
  size_t non_acc_no = binary_info->dfa_non_acc_no;
  size_t state_no = binary_info->dfa_state_no;
  size_t nfa_no = binary_info->nfa_no;
  KevPatternList* list = &parser_state->list;
  KevStringMap* env_var = &parser_state->env_var;

  char* error_handler = NULL;
  char* default_callback = NULL;
  KevStringMapNode* node = kev_strmap_search(env_var, "error-handler");
  error_handler = node ? node->value : NULL;
  node = kev_strmap_search(env_var, "default-callback");
  default_callback = node ? node->value : NULL;

  size_t i = 0;
  KevPattern* pattern = list->head->next;
  char** func_names = (char**)malloc(sizeof (char*) * nfa_no);
  if (!func_names) fatal_error("out of meory", NULL);
  while (pattern) {
    KevFAInfo* nfa_info = pattern->fa_info;
    while (nfa_info) {
      func_names[i++] = nfa_info->name ? nfa_info->name : default_callback;
      nfa_info = nfa_info->next;
    }
    pattern = pattern->next;
  }
  char** callbacks = (char**)malloc(sizeof (char*) * state_no);
  if (!callbacks) fatal_error("out of meory", NULL);
  for (size_t i = 0; i < non_acc_no; ++i) {
    callbacks[i] = error_handler;
  }
  for(size_t i = non_acc_no; i < state_no; ++i) {
    callbacks[i] = func_names[acc_mapping[i - non_acc_no]];;
  }
  free(func_names);
  binary_info->callbacks = callbacks;
}

static void kev_lexgen_convert_pattern_mapping(KevPatternBinary* binary_info, KevParserState* parser_state,
                                                    size_t* acc_mapping) {
  size_t non_acc_state_no = binary_info->dfa_non_acc_no;
  size_t nfa_no = binary_info->nfa_no;
  size_t state_no = binary_info->dfa_state_no;
  KevPatternList* list = &parser_state->list;
  KevStringMap* env_var = &parser_state->env_var;
  int error_id = -1;
  KevStringMapNode* node = kev_strmap_search(env_var, "error-id");
  if (node) {
    if ((error_id = atoi(node->value)) == 0) {
      fatal_error("invalid integer value for error-id", NULL);
    }
  }
  /* convert pattern_mapping */
  size_t* nfa_to_pattern = (size_t*)malloc(sizeof (size_t) * nfa_no);
  if (!nfa_to_pattern) fatal_error("out of memory", NULL);
  KevPattern* pattern = list->head->next;
  size_t i = 0;
  size_t pattern_id = 0;
  while (pattern) {
    KevFAInfo* nfa_info = pattern->fa_info;
    while (nfa_info) {
      nfa_to_pattern[i++] = pattern_id;
      nfa_info = nfa_info->next;
    }
    pattern_id++;
    pattern = pattern->next;
  }
  int* pattern_mapping = (int*)malloc(sizeof (size_t) * state_no);
  for (size_t i = 0; i < non_acc_state_no; ++i)
    pattern_mapping[i] = error_id;
  for (size_t i = non_acc_state_no; i < state_no; ++i)
    pattern_mapping[i] = (int)nfa_to_pattern[acc_mapping[i - non_acc_state_no]];
  free(nfa_to_pattern);
  binary_info->pattern_mapping = pattern_mapping;
}

static void kev_lexgen_convert_macro_array(KevPatternBinary* binary_info, KevParserState* parser_state) {
  KevPattern* pattern = parser_state->list.head->next;
  char** macros = (char**)malloc(sizeof (char*) * binary_info->pattern_no);
  if ( !macros)
    fatal_error("out of memory", NULL);

  size_t i = 0;
  while (pattern) {
    macros[i++] = pattern->macro;
    pattern = pattern->next;
  }
  binary_info->macros = macros;
}

static void kev_lexgen_convert_generate(KevPatternBinary* patterns_info, KevParserState* parser_state, KevFA** p_min_dfa,
                                        size_t** p_acc_mapping) {
  KevPatternList* list = & parser_state->list;
  size_t nfa_no = 0;
  KevPattern* pattern = list->head->next; /* fisrt pattern is used to store all NFA variables */
  while (pattern) {
    KevFAInfo* nfa_info = pattern->fa_info;
    while (nfa_info) {
      nfa_no++;
      nfa_info = nfa_info->next;
    }
    pattern = pattern->next;
  }

  KevFA** nfa_array = (KevFA**)malloc(sizeof (KevFA*) * (nfa_no + 1));
  if (!nfa_array) fatal_error("out of memory", NULL);
  pattern = list->head->next;
  size_t i = 0;
  size_t pattern_no = 0;
  while (pattern) {
    KevFAInfo* nfa_info = pattern->fa_info;
    while (nfa_info) {
      nfa_array[i++] = nfa_info->fa;
      nfa_info = nfa_info->next;
    }
    pattern = pattern->next;
    pattern_no++;
  }
  nfa_array[i] = NULL;  /* nfa_array must terminate with NULL */

  KevFA* dfa = kev_nfa_to_dfa(nfa_array, p_acc_mapping);
  free(nfa_array);
  if (!dfa)
    fatal_error("failed to convert dfa", NULL);
  *p_min_dfa = kev_dfa_minimization(dfa, *p_acc_mapping);
  kev_fa_delete(dfa);
  if (!*p_min_dfa)
    fatal_error("failed to minimize dfa", NULL);
  patterns_info->nfa_no = nfa_no;
  patterns_info->dfa_state_no = kev_fa_state_assign_id(*p_min_dfa, 0);
  patterns_info->dfa_non_acc_no = kev_dfa_non_accept_state_number(*p_min_dfa);
  patterns_info->dfa_start = (*p_min_dfa)->start_state->id;
  patterns_info->pattern_no = pattern_no;
}

static void kev_lexgen_convert_table(KevPatternBinary* binary_info, KevParserState* parser_state, KevFA* dfa) {
  size_t alphabet_size = atoi(kev_strmap_search(&parser_state->env_var, "alphabet-size")->value);
  size_t length = atoi(kev_strmap_search(&parser_state->env_var, "state-length")->value);
  if (alphabet_size == 128 && length == 8) {
    binary_info->table = kev_lexgen_convert_table_128_u8(dfa);
  } else if (alphabet_size == 128 && length == 16) {
    binary_info->table = kev_lexgen_convert_table_128_u16(dfa);
  } else if (alphabet_size == 256 && length == 8) {
    binary_info->table = kev_lexgen_convert_table_256_u8(dfa);
  } else if (alphabet_size == 256 && length == 16) {
    binary_info->table = kev_lexgen_convert_table_256_u16(dfa);
  } else {
    fatal_error("internal error occurred in kev_lexgen_output_table()", NULL);
  }
  if (!binary_info->table) {
    fatal_error("failed to generate transition table, try --width=16 and --charset=utf-8", NULL);
  }
  binary_info->charset_size = alphabet_size;
  binary_info->state_length = length;
}

static void kev_lexgen_convert_infos(KevPatternBinary* binary_info, KevParserState* parser_state) {
  char** infos = (char**)malloc(sizeof (char*) * binary_info->pattern_no);
  if (!infos)
    fatal_error("out of memory", NULL);
  KevPattern* pattern = parser_state->list.head->next;
  size_t i = 0;
  while (pattern) {
    infos[i++] = pattern->name;
    pattern = pattern->next;
  }
  binary_info->infos = infos;
}

uint8_t (*kev_lexgen_convert_table_256_u8(KevFA* dfa))[256] {
  size_t state_number = kev_fa_state_assign_id(dfa, 0);
  if (state_number >= 256) return NULL;
  uint8_t (*table)[256] = (uint8_t(*)[256])malloc(sizeof (*table) * state_number);
  if (!table) return NULL;
  for (size_t i = 0; i < state_number; ++i) {
    for (size_t j = 0; j < 256; ++j) {
      table[i][j] = 255;
    }
  }
  KevGraphNode* node = kev_fa_get_states(dfa);
  while (node) {
    KevGraphEdge* edge = kev_graphnode_get_edges(node);
    uint64_t id = node->id;
    while (edge) {
      if ((uint64_t)edge->attr <= 255)
        table[id][(uint8_t)edge->attr] = (uint8_t)edge->node->id;
      edge = edge->next;
    }
    node = node->next;
  }
  return table;
}

uint8_t (*kev_lexgen_convert_table_128_u8(KevFA* dfa))[128] {
  size_t state_number = kev_fa_state_assign_id(dfa, 0);
  if (state_number >= 256) return NULL;
  uint8_t (*table)[128] = (uint8_t(*)[128])malloc(sizeof (*table) * state_number);
  if (!table) return NULL;
  for (size_t i = 0; i < state_number; ++i) {
    for (size_t j = 0; j < 128; ++j) {
      table[i][j] = 127;
    }
  }
  KevGraphNode* node = kev_fa_get_states(dfa);
  while (node) {
    KevGraphEdge* edge = kev_graphnode_get_edges(node);
    uint64_t id = node->id;
    while (edge) {
      if ((uint64_t)edge->attr <= 127)
        table[id][(uint8_t)edge->attr] = (uint8_t)edge->node->id;
      edge = edge->next;
    }
    node = node->next;
  }
  return table;
}

uint16_t (*kev_lexgen_convert_table_256_u16(KevFA* dfa))[256] {
  size_t state_number = kev_fa_state_assign_id(dfa, 0);
  if (state_number >= 65536) return NULL;
  uint16_t (*table)[256] = (uint16_t(*)[256])malloc(sizeof (*table) * state_number);
  if (!table) return NULL;
  for (size_t i = 0; i < state_number; ++i) {
    for (size_t j = 0; j < 256; ++j) {
      table[i][j] = 255;
    }
  }
  KevGraphNode* node = kev_fa_get_states(dfa);
  while (node) {
    KevGraphEdge* edge = kev_graphnode_get_edges(node);
    uint64_t id = node->id;
    while (edge) {
      if ((uint64_t)edge->attr <= 255)
        table[id][(uint16_t)edge->attr] = (uint16_t)edge->node->id;
      edge = edge->next;
    }
    node = node->next;
  }
  return table;
}

uint16_t (*kev_lexgen_convert_table_128_u16(KevFA* dfa))[128] {
  size_t state_number = kev_fa_state_assign_id(dfa, 0);
  if (state_number >= 65536) return NULL;
  uint16_t (*table)[128] = (uint16_t(*)[128])malloc(sizeof (*table) * state_number);
  if (!table) return NULL;
  for (size_t i = 0; i < state_number; ++i) {
    for (size_t j = 0; j < 128; ++j) {
      table[i][j] = 127;
    }
  }
  KevGraphNode* node = kev_fa_get_states(dfa);
  while (node) {
    KevGraphEdge* edge = kev_graphnode_get_edges(node);
    uint64_t id = node->id;
    while (edge) {
      if ((uint64_t)edge->attr <= 127)
        table[id][(uint16_t)edge->attr] = (uint16_t)edge->node->id;
      edge = edge->next;
    }
    node = node->next;
  }
  return table;
}

static void fatal_error(char* info, char* info2) {
  fputs("fatal: ", stderr);
  if (info)
    fputs(info, stderr);
  if (info2)
    fputs(info2, stderr);
  fputs("\nterminated\n", stderr);
  exit(EXIT_FAILURE);
}
