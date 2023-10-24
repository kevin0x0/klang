#ifndef KEVCC_KEVLR_INCLUDE_TABLE_H
#define KEVCC_KEVLR_INCLUDE_TABLE_H

#include "kevlr/include/collection.h"

#define KLR_ACTION_ERR     ((KlrAction)0)
#define KLR_ACTION_SHI     ((KlrAction)1)
#define KLR_ACTION_RED     ((KlrAction)2)
#define KLR_ACTION_ACC     ((KlrAction)3)
#define KLR_ACTION_CON     ((KlrAction)4)

#define KLR_GOTO_NONE      ((KlrTransitionEntry)-1)


struct tagKlrTableEntry;

typedef struct tagKlrConflict {
  KlrItemSet* itemset;
  KlrSymbol* symbol;
  KlrItemSet* conflict_items;
  struct tagKlrTableEntry* entry;
  struct tagKlrConflict* next;
} KlrConflict;

typedef union tagKlrActionInfo {
  KlrRule* rule;
  KlrConflict* conflict;
  size_t itemset_id;
} KlrActionInfo;

typedef int64_t KlrTransitionEntry;
typedef uint8_t KlrAction;

typedef struct tagKlrTableEntry {
  KlrActionInfo info;
  KlrTransitionEntry trans;
  KlrAction action;
} KlrTableEntry;


typedef struct tagKlrTable {
  KlrTableEntry** entries;
  size_t start_state;
  size_t state_no;
  size_t total_symbol_no;
  size_t table_symbol_no;
  size_t terminal_no;
  size_t max_terminal_id;
  KlrConflict* conflicts;
} KlrTable;

typedef bool KlrConflictCallback(void* object, KlrConflict* conflict, KlrCollection* collec);

/* conflict handler */
typedef struct tagKlrConflictHandler {
  void* object;
  KlrConflictCallback* callback;
} KlrConflictHandler;

/* generation of table */
KlrTable* klr_table_create(KlrCollection* collec, KlrConflictHandler* conf_handler);
void klr_table_delete(KlrTable* table);

static inline KlrConflict* klr_table_get_conflict(KlrTable* table);
static inline size_t klr_table_get_symbol_no(KlrTable* table);
static inline size_t klr_table_get_terminal_no(KlrTable* table);
static inline size_t klr_table_get_state_no(KlrTable* table);
static inline size_t klr_table_get_start_state(KlrTable* table);
static inline KlrConflict* klr_table_conflict_next(KlrConflict* conflict);

static inline KlrTransitionEntry klr_table_get_trans(KlrTable* table, KlrID itemset_id, KlrID symbol_id);
static inline KlrAction klr_table_get_action(KlrTable* table, KlrID itemset_id, KlrID symbol_id);
static inline KlrActionInfo* klr_table_get_action_info(KlrTable* table, KlrID itemset_id, KlrID symbol_id);

/* conflict */
KlrConflict* klr_conflict_create(KlrItemSet* itemset, KlrSymbol* symbol, KlrTableEntry* entry);
void klr_conflict_delete(KlrConflict* conflict);
static inline void klr_conflict_add_item(KlrConflict* conflict, KlrItem* item);
static inline bool klr_conflict_handle(KlrConflictHandler* handler, KlrConflict* conflict, KlrCollection* collec);

/* conflict handling */
static inline bool klr_conflict_SR(KlrConflict* conflict);
static inline bool klr_conflict_RR(KlrConflict* conflict);
static inline KlrItemSet* klr_conflict_get_conflict_items(KlrConflict* conflict);
static inline KlrItemSet* klr_conflict_get_itemset(KlrConflict* conflict);
static inline KlrSymbol* klr_conflict_get_symbol(KlrConflict* conflict);
static inline size_t klr_conflict_get_status(KlrConflict* conflict);
static inline void klr_conflict_set_reducing(KlrConflict* conflict, KlrCollection* collec, KlrItem* item);
static inline void klr_conflict_set_shifting(KlrConflict* conflict);

/* conflict handler */
KlrConflictHandler* klr_conflict_handler_create(void* object, KlrConflictCallback* callback);
void klr_conflict_handler_delete(KlrConflictHandler* handler);



/* implementation of inline functions */
static inline KlrConflict* klr_table_get_conflict(KlrTable* table) {
  return table->conflicts;
}

static inline size_t klr_table_get_symbol_no(KlrTable* table) {
  return table->table_symbol_no;
}

static inline size_t klr_table_get_terminal_no(KlrTable* table) {
  return table->terminal_no;
}

static inline size_t klr_table_get_max_terminal_id(KlrTable* table) {
  return table->max_terminal_id;
}

static inline size_t klr_table_get_state_no(KlrTable* table) {
  return table->state_no;
}

static inline size_t klr_table_get_start_state(KlrTable* table) {
  return table->start_state;
}

static inline KlrConflict* klr_table_conflict_next(KlrConflict* conflict) {
  return conflict->next;
}

static inline KlrTransitionEntry klr_table_get_trans(KlrTable* table, KlrID itemset_id, KlrID symbol_id) {
  return table->entries[itemset_id][symbol_id].trans;
}

static inline KlrAction klr_table_get_action(KlrTable* table, KlrID itemset_id, KlrID symbol_id) {
  return table->entries[itemset_id][symbol_id].action;
}

static inline KlrActionInfo* klr_table_get_action_info(KlrTable* table, KlrID itemset_id, KlrID symbol_id) {
  return &table->entries[itemset_id][symbol_id].info;
}

static inline void klr_conflict_add_item(KlrConflict* conflict, KlrItem* item) {
  klr_itemset_add_item(conflict->conflict_items, item);
}

static inline bool klr_conflict_SR(KlrConflict* conflict) {
  return conflict->entry->trans != KLR_GOTO_NONE;
}

static inline bool klr_conflict_RR(KlrConflict* conflict) {
  return conflict->conflict_items->items->next != NULL;
}

static inline KlrItemSet* klr_conflict_get_conflict_items(KlrConflict* conflict) {
  return conflict->conflict_items;
}

static inline KlrItemSet* klr_conflict_get_itemset(KlrConflict* conflict) {
  return conflict->itemset;
}

static inline KlrSymbol* klr_conflict_get_symbol(KlrConflict* conflict) {
  return conflict->symbol;
}

static inline size_t klr_conflict_get_status(KlrConflict* conflict) {
  return conflict->entry->action;
}

static inline bool klr_conflict_handle(KlrConflictHandler* handler, KlrConflict* conflict, KlrCollection* collec) {
  return handler->callback(handler->object, conflict, collec);
}

static inline void klr_conflict_set_reducing(KlrConflict* conflict, KlrCollection* collec, KlrItem* item) {
  KlrTableEntry* entry = conflict->entry;
  KlrRule* rule = klr_item_get_rule(item);
  entry->action = klr_collection_get_start_rule(collec) == rule ?
                  KLR_ACTION_ACC : KLR_ACTION_RED;
  entry->info.rule = rule;
}

static inline void klr_conflict_set_shifting(KlrConflict* conflict) {
  KlrTableEntry* entry = conflict->entry;
  entry->action = KLR_ACTION_SHI;
  entry->info.itemset_id = entry->trans;
}

#endif
