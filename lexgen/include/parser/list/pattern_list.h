#ifndef KEVCC_LEXGEN_INCLUDE_PARSER_LIST_PATTERN_LIST_H
#define KEVCC_LEXGEN_INCLUDE_PARSER_LIST_PATTERN_LIST_H

#include "lexgen/include/finite_automaton/finite_automaton.h"

typedef struct tagKevFAInfo {
  KevFA* fa;
  char* name;
  struct tagKevFAInfo* next;
} KevFAInfo;

typedef struct tagKevPattern {
  char* name;
  char* macro;
  KevFAInfo* fa_info;
  struct tagKevPattern* next;
} KevPattern;

typedef struct tagKevPatternList {
  KevPattern* head;
  KevPattern* tail;
} KevPatternList;

bool kev_patternlist_init(KevPatternList* list);
void kev_patternlist_destroy(KevPatternList* list);
bool kev_patternlist_insert(KevPatternList* list, char* pattern_name, char* macro);
bool kev_pattern_insert(KevPattern* info, char* name, KevFA* fa);

void kev_patternlist_free_content(KevPatternList* list);

#endif
