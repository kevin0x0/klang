#ifndef KEVCC_PARGEN_INCLUDE_LR_TABLE_H
#define KEVCC_PARGEN_INCLUDE_LR_TABLE_H

#include "pargen/include/lr/collection.h"

#define KEV_LR_ACTION_ERR     (0)
#define KEV_LR_ACTION_SHI     (1)
#define KEV_LR_ACTION_RED     (2)
#define KEV_LR_ACTION_ACC     (3)
#define KEV_LR_ACTION_CON     (4)

#define KEV_LR_GOTO_NONE      ((KevLRGotoEntry)-1)


struct tagKevLRTableEntry;

typedef struct tagKevLRConflict {
  KevItemSet* itemset;
  KevSymbol* symbol;
  KevItemSet* conflct_items;
  struct tagKevLRTableEntry* entry;
  struct tagKevLRConflict* next;
} KevLRConflict;

typedef union tagKevLRActionInfo {
  KevRule* rule;
  KevLRConflict* conflict;
  size_t itemset_id;
} KevLRActionInfo;

typedef int64_t KevLRGotoEntry;

typedef struct tagKevLRTableEntry {
  KevLRActionInfo info;
  KevLRGotoEntry go_to;
  int action;
} KevLRTableEntry;


typedef struct tagKevLRTable {
  KevLRTableEntry** entries;
  size_t itemset_no;
  size_t symbol_no;
  size_t terminal_no;
  KevLRConflict* conflicts;
} KevLRTable;

typedef bool KevLRConflictCallback(void* object, KevLRConflict* conflict, KevLRCollection* collec);

/* conflict handler */
typedef struct tagKevLRConflictHandler {
  void* object;
  KevLRConflictCallback* callback;
} KevLRConflictHandler;

/* generation of table */
KevLRTable* kev_lr_table_create(KevLRCollection* collec, KevLRConflictHandler* conf_handler);
void kev_lr_table_delete(KevLRTable* table);

/* conflict */
KevLRConflict* kev_lr_conflict_create(KevItemSet* itemset, KevSymbol* symbol, KevLRTableEntry* entry);
void kev_lr_conflict_delete(KevLRConflict* conflict);
static inline void kev_lr_conflict_add_item(KevLRConflict* conflict, KevItem* item);
static inline bool kev_lr_conflict_handle(KevLRConflictHandler* handler, KevLRConflict* conflict, KevLRCollection* collec);
static inline bool kev_lr_conflict_SR(KevLRConflict* conflict);
static inline bool kev_lr_conflict_RR(KevLRConflict* conflict);

/* conflict handler */
KevLRConflictHandler* kev_lr_conflict_handler_create(void* object, KevLRConflictCallback* callback);
void kev_lr_conflict_handler_delete(KevLRConflictHandler* handler);


/* implement of inline functions */
static inline void kev_lr_conflict_add_item(KevLRConflict* conflict, KevItem* item) {
  kev_lr_itemset_add_item(conflict->conflct_items, item);
}

static inline bool kev_lr_conflict_SR(KevLRConflict* conflict) {
  return conflict->entry->go_to != KEV_LR_GOTO_NONE;
}

static inline bool kev_lr_conflict_RR(KevLRConflict* conflict) {
  return conflict->conflct_items->items->next != NULL;
}

static inline bool kev_lr_conflict_handle(KevLRConflictHandler* handler, KevLRConflict* conflict, KevLRCollection* collec) {
  return handler->callback(handler->object, conflict, collec);
}

#endif
