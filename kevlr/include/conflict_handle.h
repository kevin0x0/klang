#ifndef KEVCC_KEVLR_INCLUDE_CONFLICT_HANDLE_H
#define KEVCC_KEVLR_INCLUDE_CONFLICT_HANDLE_H

#include "kevlr/include/table.h"

extern KevLRConflictHandler* kev_lr_confhandler_reducing;
extern KevLRConflictHandler* kev_lr_confhandler_shifting;

static inline KevLRConflictCallback kev_lr_confhandler_shifting_callback;
static inline KevLRConflictCallback kev_lr_confhandler_reducing_callback;
KevLRConflictCallback kev_lr_confhandler_priority_callback;


static inline bool kev_lr_confhandler_reducing_callback(void* object, KevLRConflict* conflict, KevLRCollection* collec) {
  if (kev_lr_conflict_RR(conflict)) return false;
  kev_lr_conflict_set_reducing(conflict, collec, conflict->conflict_items->items);
  return true;
}

static inline bool kev_lr_confhandler_shifting_callback(void* object, KevLRConflict* conflict, KevLRCollection* collec) {
  if (kev_lr_conflict_RR(conflict)) return false;
  kev_lr_conflict_set_shifting(conflict);
  return true;
}

#endif
