#ifndef KEVCC_KEVLR_INCLUDE_TABLE_H
#define KEVCC_KEVLR_INCLUDE_TABLE_H

#include "kevlr/include/collection.h"

#define KEV_LR_ACTION_ERR     ((KevLRAction)0)
#define KEV_LR_ACTION_SHI     ((KevLRAction)1)
#define KEV_LR_ACTION_RED     ((KevLRAction)2)
#define KEV_LR_ACTION_ACC     ((KevLRAction)3)
#define KEV_LR_ACTION_CON     ((KevLRAction)4)

#define KEV_LR_GOTO_NONE      ((KevLRGotoEntry)-1)


struct tagKevLRTableEntry;

typedef struct tagKevLRConflict {
  KevItemSet* itemset;
  KevSymbol* symbol;
  KevItemSet* conflict_items;
  struct tagKevLRTableEntry* entry;
  struct tagKevLRConflict* next;
} KevLRConflict;

typedef union tagKevLRActionInfo {
  KevRule* rule;
  KevLRConflict* conflict;
  size_t itemset_id;
} KevLRActionInfo;

typedef int64_t KevLRGotoEntry;
typedef uint8_t KevLRAction;

typedef struct tagKevLRTableEntry {
  KevLRActionInfo info;
  KevLRGotoEntry go_to;
  KevLRAction action;
} KevLRTableEntry;


typedef struct tagKevLRTable {
  KevLRTableEntry** entries;
  size_t start_state;
  size_t state_no;
  size_t total_symbol_no;
  size_t table_symbol_no;
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

static inline KevLRConflict* kev_lr_table_get_conflict(KevLRTable* table);
static inline size_t kev_lr_table_get_symbol_no(KevLRTable* table);
static inline size_t kev_lr_table_get_terminal_no(KevLRTable* table);
static inline size_t kev_lr_table_get_state_no(KevLRTable* table);
static inline size_t kev_lr_table_get_start_state(KevLRTable* table);
static inline KevLRConflict* kev_lr_table_conflict_next(KevLRConflict* conflict);

static inline KevLRGotoEntry kev_lr_table_get_goto(KevLRTable* table, KevLRID itemset_id, KevLRID symbol_id);
static inline KevLRAction kev_lr_table_get_action(KevLRTable* table, KevLRID itemset_id, KevLRID symbol_id);
static inline KevLRActionInfo* kev_lr_table_get_action_info(KevLRTable* table, KevLRID itemset_id, KevLRID symbol_id);

/* conflict */
KevLRConflict* kev_lr_conflict_create(KevItemSet* itemset, KevSymbol* symbol, KevLRTableEntry* entry);
void kev_lr_conflict_delete(KevLRConflict* conflict);
static inline void kev_lr_conflict_add_item(KevLRConflict* conflict, KevItem* item);
static inline bool kev_lr_conflict_handle(KevLRConflictHandler* handler, KevLRConflict* conflict, KevLRCollection* collec);

/* conflict interface */
static inline bool kev_lr_conflict_SR(KevLRConflict* conflict);
static inline bool kev_lr_conflict_RR(KevLRConflict* conflict);
static inline KevItemSet* kev_lr_conflict_get_conflict_items(KevLRConflict* conflict);
static inline KevItemSet* kev_lr_conflict_get_itemset(KevLRConflict* conflict);
static inline KevSymbol* kev_lr_conflict_get_symbol(KevLRConflict* conflict);
static inline size_t kev_lr_conflict_get_status(KevLRConflict* conflict);
static inline void kev_lr_conflict_set_reducing(KevLRConflict* conflict, KevLRCollection* collec, KevItem* item);
static inline void kev_lr_conflict_set_shifting(KevLRConflict* conflict);

/* conflict handler */
KevLRConflictHandler* kev_lr_conflict_handler_create(void* object, KevLRConflictCallback* callback);
void kev_lr_conflict_handler_delete(KevLRConflictHandler* handler);



/* implementation of inline functions */
static inline KevLRConflict* kev_lr_table_get_conflict(KevLRTable* table) {
  return table->conflicts;
}

static inline size_t kev_lr_table_get_symbol_no(KevLRTable* table) {
  return table->table_symbol_no;
}

static inline size_t kev_lr_table_get_terminal_no(KevLRTable* table) {
  return table->terminal_no;
}

static inline size_t kev_lr_table_get_state_no(KevLRTable* table) {
  return table->state_no;
}

static inline size_t kev_lr_table_get_start_state(KevLRTable* table) {
  return table->start_state;
}

static inline KevLRConflict* kev_lr_table_conflict_next(KevLRConflict* conflict) {
  return conflict->next;
}

static inline KevLRGotoEntry kev_lr_table_get_goto(KevLRTable* table, KevLRID itemset_id, KevLRID symbol_id) {
  return table->entries[itemset_id][symbol_id].go_to;
}

static inline KevLRAction kev_lr_table_get_action(KevLRTable* table, KevLRID itemset_id, KevLRID symbol_id) {
  return table->entries[itemset_id][symbol_id].action;
}

static inline KevLRActionInfo* kev_lr_table_get_action_info(KevLRTable* table, KevLRID itemset_id, KevLRID symbol_id) {
  return &table->entries[itemset_id][symbol_id].info;
}

static inline void kev_lr_conflict_add_item(KevLRConflict* conflict, KevItem* item) {
  kev_lr_itemset_add_item(conflict->conflict_items, item);
}

static inline bool kev_lr_conflict_SR(KevLRConflict* conflict) {
  return conflict->entry->go_to != KEV_LR_GOTO_NONE;
}

static inline bool kev_lr_conflict_RR(KevLRConflict* conflict) {
  return conflict->conflict_items->items->next != NULL;
}

static inline KevItemSet* kev_lr_conflict_get_conflict_items(KevLRConflict* conflict) {
  return conflict->conflict_items;
}

static inline KevItemSet* kev_lr_conflict_get_itemset(KevLRConflict* conflict) {
  return conflict->itemset;
}

static inline KevSymbol* kev_lr_conflict_get_symbol(KevLRConflict* conflict) {
  return conflict->symbol;
}

static inline size_t kev_lr_conflict_get_status(KevLRConflict* conflict) {
  return conflict->entry->action;
}

static inline bool kev_lr_conflict_handle(KevLRConflictHandler* handler, KevLRConflict* conflict, KevLRCollection* collec) {
  return handler->callback(handler->object, conflict, collec);
}

static inline void kev_lr_conflict_set_reducing(KevLRConflict* conflict, KevLRCollection* collec, KevItem* item) {
  KevLRTableEntry* entry = conflict->entry;
  KevRule* rule = kev_lr_item_get_rule(item);
  entry->action = kev_lr_collection_get_start_rule(collec) == rule ?
                  KEV_LR_ACTION_ACC : KEV_LR_ACTION_RED;
  entry->info.rule = rule;
}

static inline void kev_lr_conflict_set_shifting(KevLRConflict* conflict) {
  KevLRTableEntry* entry = conflict->entry;
  entry->action = KEV_LR_ACTION_SHI;
  entry->info.itemset_id = entry->go_to;
}

#endif
