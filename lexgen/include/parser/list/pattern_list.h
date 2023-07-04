#ifndef KEVCC_LEXGEN_INCLUDE_PARSER_LIST_PATTERN_LIST_H
#define KEVCC_LEXGEN_INCLUDE_PARSER_LIST_PATTERN_LIST_H
#include "lexgen/include/finite_automaton/finite_automaton.h"

typedef struct tagKevFAInfo {
  KevFA* fa;
  char* name;
  struct tagKevFAInfo* next;
} KevFAInfo;

typedef struct tagKevPatternInfo {
  char* name;
  KevFAInfo* fa_info;
  struct tagKevPatternInfo* next;
} KevPatternInfo;

typedef struct tagKevPatternList {
  KevPatternInfo* head;
  KevPatternInfo* tail;
} KevPatternList;

bool kev_patternlist_init(KevPatternList* list);
void kev_patternlist_destroy(KevPatternList* list);
bool kev_patternlist_insert(KevPatternList* list, char* pattern_name);
bool kev_patterninfo_insert(KevPatternInfo* info, char* name, KevFA* fa);

void kev_patternlist_free_content(KevPatternList* list);

#endif
