#include "lexgen/include/parser/list/pattern_list.h"
#include "lexgen/include/finite_automaton/finite_automaton.h"

#include <stdlib.h>

static void kev_patterninfo_destroy(KevPattern* info);
static void kev_patterninfo_free_content(KevPattern* info);

bool kev_patternlist_init(KevPatternList* list) {
  if (!list) return false;
  list->head = (KevPattern*)malloc(sizeof (KevPattern));
  list->tail = list->head;
  if (!list->head) return false;
  list->head->name = NULL;
  list->head->next = NULL;
  list->head->fa_info = NULL;
  return true;
}

void kev_patternlist_destroy(KevPatternList* list) {
  KevPattern* head = list->head;
  while (head) {
    kev_patterninfo_destroy(head);
    KevPattern* tmp = head->next;
    free(head);
    head = tmp;
  }
}

bool kev_patternlist_insert(KevPatternList* list, char* pattern_name) {
  KevPattern* tail = (KevPattern*)malloc(sizeof (KevPattern));
  if (!tail) return false;
  tail->name = pattern_name;
  tail->fa_info = NULL;
  tail->next = NULL;
  list->tail->next = tail;
  list->tail = tail;
  return true;
}

bool kev_pattern_insert(KevPattern* info, char* name, KevFA* fa) {
  KevFAInfo* new_fa_info = (KevFAInfo*)malloc(sizeof (KevFAInfo));
  if (!new_fa_info) return false;
  new_fa_info->name = name;
  new_fa_info->fa = fa;
  new_fa_info->next = info->fa_info;
  info->fa_info = new_fa_info;
  return true;
}

void kev_patternlist_free_content(KevPatternList* list) {
  KevPattern* head = list->head;
  while (head) {
    kev_patterninfo_free_content(head);
    head = head->next;
  }
}

static void kev_patterninfo_destroy(KevPattern* info) {
  KevFAInfo* fa_info = info->fa_info;
  while (fa_info) {
    KevFAInfo* tmp = fa_info->next;
    free(fa_info);
    fa_info = tmp;
  }
}

static void kev_patterninfo_free_content(KevPattern* info) {
  KevFAInfo* fa_info = info->fa_info;
  while (fa_info) {
    free(fa_info->name);
    fa_info->name = NULL;
    kev_fa_delete(fa_info->fa);
    fa_info->fa = NULL;
    fa_info = fa_info->next;
  }
  free(info->name);
  info->name = NULL;
}
