#ifndef KEVCC_PARGEN_INCLUDE_LR_TABLE_H
#define KEVCC_PARGEN_INCLUDE_LR_TABLE_H

#include "pargen/include/lr/collection.h"

#define KEV_LR_ACTION_ERR     (0)
#define KEV_LR_ACTION_SHI     (1)
#define KEV_LR_ACTION_RED     (2)
#define KEV_LR_ACTION_ACC     (3)
#define KEV_LR_ACTION_CON     (4)

#define KEV_LR_GOTO_NONE      ((KevLRGotoEntry)-1)

#define KEV_LR_CONFLICT_RR    (0)
#define KEV_LR_CONFLICT_SR    (1)

typedef struct tagKevLRConflict {
  KevItemSet* conflict_itemset;
  KevSymbol* symbol;
  struct tagKevLRConflict* next;
  int type;
} KevLRConflict;

typedef union tagKevLRActionEntryInfo {
  KevRule* rule;
  KevLRConflict* conflict;
  size_t itemset;
} KevLRActionEntryInfo;

typedef struct tagKevLRActionEntry {
  KevLRActionEntryInfo info;
  int action;
} KevLRActionEntry;

typedef int64_t KevLRGotoEntry;

typedef struct tagKevLRTable {
  KevLRGotoEntry** gotos;
  KevLRActionEntry** actions;
  size_t itemset_no;
  size_t symbol_no;
  size_t terminal_no;
  KevLRConflict* conflicts;
} KevLRTable;



/* generation of table */
KevLRTable* kev_lr_table_create(KevLRCollection* collec);
void kev_lr_table_delete(KevLRTable* table);

/* conflict */
KevLRConflict* kev_lr_conflict_create(KevItemSet* itemset, KevSymbol* symbol, int conflict_type);
static inline void kev_lr_conflict_delete(KevLRConflict* conflict);

static inline void kev_lr_conflict_delete(KevLRConflict* conflict) {
  free(conflict);
}

#endif
