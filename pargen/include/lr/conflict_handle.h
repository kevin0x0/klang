#ifndef KEVCC_PARGEN_INCLUDE_LR_CONFLICT_HANDLE_H
#define KEVCC_PARGEN_INCLUDE_LR_CONFLICT_HANDLE_H

#include "pargen/include/lr/table.h"

extern KevLRConflictHandler* kev_lr_confhandler_reducing;
extern KevLRConflictHandler* kev_lr_confhandler_shifting;

static inline void kev_lr_conflict_set_reducing(KevLRConflict* conflict, KevItem* item);
static inline void kev_lr_conflict_set_shifting(KevLRConflict* conflict);

static inline KevLRConflictCallback kev_lr_confhandler_shifting_callback;
static inline KevLRConflictCallback kev_lr_confhandler_reducing_callback;
/* TODO: immplement priority handler callback */
KevLRConflictCallback kev_lr_confhandler_priority_callback;


static inline void kev_lr_conflict_set_reducing(KevLRConflict* conflict, KevItem* item) {
  KevLRTableEntry* entry = conflict->entry;
  entry->action = KEV_LR_ACTION_RED;
  entry->info.rule = item->rule;
}

static inline void kev_lr_conflict_set_shifting(KevLRConflict* conflict) {
  KevLRTableEntry* entry = conflict->entry;
  entry->action = KEV_LR_ACTION_SHI;
  size_t target_itemset_id = entry->info.conflict->itemset->id;
  entry->info.itemset_id = target_itemset_id;
}

static inline bool kev_lr_confhandler_reducing_callback(void* object, KevLRConflict* conflict, KevLRCollection* collec) {
  if (kev_lr_conflict_RR(conflict)) return false;
  kev_lr_conflict_set_reducing(conflict, conflict->conflct_items->items);
  return true;
}

static inline bool kev_lr_confhandler_shifting_callback(void* object, KevLRConflict* conflict, KevLRCollection* collec) {
  if (kev_lr_conflict_RR(conflict)) return false;
  kev_lr_conflict_set_shifting(conflict);
  return true;
}

#endif
