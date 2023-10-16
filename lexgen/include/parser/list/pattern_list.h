#ifndef KEVCC_LEXGEN_INCLUDE_PARSER_LIST_PATTERN_LIST_H
#define KEVCC_LEXGEN_INCLUDE_PARSER_LIST_PATTERN_LIST_H

#include "kevfa/include/finite_automaton.h"
#include "utils/include/array/karray.h"

typedef struct tagKevFAInfo {
  KevFA* fa;
  char* name;
  struct tagKevFAInfo* next;
} KevFAInfo;

typedef struct tagKevPattern {
  char* name;
  KArray* macros;
  size_t pattern_id;
  KevFAInfo* fa_info;
  struct tagKevPattern* next;
} KevPattern;

typedef struct tagKevPatternList {
  KevPattern* head;
  KevPattern* tail;
  size_t pattern_no;
} KevPatternList;

bool kev_patternlist_init(KevPatternList* list);
void kev_patternlist_destroy(KevPatternList* list);
bool kev_patternlist_insert(KevPatternList* list, char* pattern_name, KArray* macros, size_t pattern_id);
bool kev_pattern_insert(KevPattern* pattern, char* name, KevFA* fa);

static inline int kev_patternlist_pattern_no(KevPatternList* list);

void kev_patternlist_free_content(KevPatternList* list);

static inline int kev_patternlist_pattern_no(KevPatternList* list) {
  return list->pattern_no;
}

#endif
