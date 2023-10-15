#ifndef KEVCC_KEVLR_INCLUDE_CONFLICT_HANDLE_H
#define KEVCC_KEVLR_INCLUDE_CONFLICT_HANDLE_H

#include "kevlr/include/table.h"

extern KlrConflictHandler* klr_confhandler_reducing;
extern KlrConflictHandler* klr_confhandler_shifting;

static inline KlrConflictCallback klr_confhandler_shifting_callback;
static inline KlrConflictCallback klr_confhandler_reducing_callback;
KlrConflictCallback klr_confhandler_priority_callback;


static inline bool klr_confhandler_reducing_callback(void* object, KlrConflict* conflict, KlrCollection* collec) {
  if (klr_conflict_RR(conflict)) return false;
  klr_conflict_set_reducing(conflict, collec, conflict->conflict_items->items);
  return true;
}

static inline bool klr_confhandler_shifting_callback(void* object, KlrConflict* conflict, KlrCollection* collec) {
  if (klr_conflict_RR(conflict)) return false;
  klr_conflict_set_shifting(conflict);
  return true;
}

#endif
