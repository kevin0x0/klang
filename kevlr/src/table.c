#include "kevlr/include/table.h"
#include "kevlr/include/itemset_def.h"
#include "kevlr/include/lr_utils.h"

#include <stdlib.h>

static bool klr_decide_action(KlrCollection* collec, KlrTable* table, KlrItemSet* itemset,
                                 KlrItemSetClosure* closure, KlrConflictHandler* conf_handler);
static bool klr_init_reducing_action(KlrTable* table, KlrCollection* collec, KlrConflictHandler* conf_handler);
static bool klr_init_transition_and_shifting_action(KlrTable* table, KlrCollection* collec);
static inline void klr_table_add_conflict(KlrTable* table, KlrConflict* conflict);
static KlrTableEntry** klr_table_get_initial_entries(size_t nsymbol, size_t nitemset);
static bool klr_conflict_create_and_add_item(KlrItemPool* pool, KlrConflict* conflict, KlrRule* rule, size_t dot);

KlrTable* klr_table_create(KlrCollection* collec, KlrConflictHandler* conf_handler) {
  KlrTable* table = (KlrTable*)malloc(sizeof (KlrTable));
  if (!table) return NULL;
  /* start symbol is excluded in the table, so the actual symbol number for
   * the table is table->symbol_no - 1. */
  table->ntblsymbol = klr_util_user_symbol_max_id(collec) + 1;
  table->max_terminal_id = klr_util_user_terminal_max_id(collec);
  table->total_symbol = klr_collection_nsymbol(collec);
  table->nterminal = klr_collection_nterminal(collec);
  table->nstate = klr_collection_nitemset(collec);
  table->start_state = klr_itemset_get_id(klr_collection_get_start_itemset(collec));
  table->entries = klr_table_get_initial_entries(table->ntblsymbol, table->nstate);
  table->conflicts = NULL;
  if (!table->entries) {
    klr_table_delete(table);
    return NULL;
  }

  if (!klr_init_transition_and_shifting_action(table, collec) ||
      !klr_init_reducing_action(table, collec, conf_handler)) {
    klr_table_delete(table);
    return NULL;
  }
  table->pool = collec->pool;
  klr_itempoolcollec_init(&collec->pool);
  return table;
}

static bool klr_init_reducing_action(KlrTable* table, KlrCollection* collec, KlrConflictHandler* conf_handler) {
  size_t total_symbol = table->total_symbol;
  size_t nterminal = table->nterminal;
  size_t nitemset = table->nstate;
  KlrItemSet** itemsets = collec->itemsets;
  KlrItemSetClosure* closure = klr_closure_create(total_symbol);
  if (!closure) return false;

  for (size_t i = 0; i < nitemset; ++i) {
    if (!klr_closure_make(closure, itemsets[i], collec->firsts, nterminal) ||
        !klr_decide_action(collec, table, itemsets[i], closure, conf_handler)) {
      klr_closure_delete(closure);
      return false;
    }
    klr_closure_make_empty(closure);
  }
  klr_closure_delete(closure);
  return true;
}

static bool klr_decide_action(KlrCollection* collec, KlrTable* table, KlrItemSet* itemset,
                                 KlrItemSetClosure* closure, KlrConflictHandler* conf_handler) {
  size_t itemset_id = itemset->id;
  KlrTableEntry** entries = table->entries;
  KlrSymbol** symbols = klr_collection_get_symbols(collec);
  KlrRule* start_rule = collec->start_rule;
  KArray* closure_symbols = closure->symbols;
  KBitSet** las = closure->lookaheads;
  /* for kernel item */
  for (KlrItem* kitem = itemset->items; kitem; kitem = kitem->next) {
    KlrRule* rule = kitem->rule;
    if (kitem->dot != rule->bodylen) continue;
    /* reduce */
    KBitSet* la = kitem->lookahead;
    /* lookahead set is ensured to be not empty */
    size_t next_symbol_index = kbitset_iter_begin(la);
    size_t symbol_index = 0;
    do {
      symbol_index = next_symbol_index;
      size_t id = symbols[symbol_index]->id;
      KlrTableEntry* entry = &entries[itemset_id][id];
      if (entry->action == KLR_ACTION_CON) {
        if (!klr_conflict_create_and_add_item(&collec->pool.itempool, entry->info.conflict, kitem->rule, kitem->dot))
          return false;
        continue;
      } else if (entry->action != KLR_ACTION_ERR) {
        KlrConflict* conflict = klr_conflict_create(&collec->pool.itemsetpool, itemset, collec->symbols[symbol_index], entry);
        if (!conflict) return false;
        if (!klr_conflict_create_and_add_item(&collec->pool.itempool, conflict, kitem->rule, kitem->dot) ||
            (entry->action == KLR_ACTION_RED &&
            !klr_conflict_create_and_add_item(&collec->pool.itempool, conflict, entry->info.rule, entry->info.rule->bodylen))) {
          klr_conflict_delete(&collec->pool, conflict);
          return false;
        }
        entry->action = KLR_ACTION_CON;
        entry->info.conflict = conflict;
        if (!conf_handler || !klr_conflict_handle(conf_handler, conflict, collec))
          klr_table_add_conflict(table, conflict);
        else
          klr_conflict_delete(&collec->pool, conflict);
      } else {
        entry->action = rule == start_rule ? KLR_ACTION_ACC : KLR_ACTION_RED;
        entry->info.rule = rule;
      }
      next_symbol_index = kbitset_iter_next(la, symbol_index);
    } while (next_symbol_index != symbol_index);
  }
  /* for non-kernel item */
  size_t closure_size = karray_size(closure_symbols);
  for (size_t i = 0; i < closure_size; ++i) {
    KlrSymbol* head = (KlrSymbol*)karray_access(closure_symbols, i);
    KBitSet* la = las[head->index];
    for (KlrRuleNode* node = head->rules; node; node = node->next) {
      KlrRule* rule = node->rule;
      if (rule->bodylen != 0) continue;
      /* reduce */
      /* lookahead is guaranteed to be not empty. */
      size_t next_symbol_index = kbitset_iter_begin(la);
      size_t symbol_index = 0;
      do {
        symbol_index = next_symbol_index;
        size_t id = symbols[symbol_index]->id;
        KlrTableEntry* entry = &entries[itemset_id][id];
        if (entry->action == KLR_ACTION_CON) {
          if (!klr_conflict_create_and_add_item(&collec->pool.itempool, entry->info.conflict, rule, rule->bodylen))
            return false;
          continue;
        } else if (entry->action != KLR_ACTION_ERR) {
          KlrConflict* conflict = klr_conflict_create(&collec->pool.itemsetpool, itemset, collec->symbols[symbol_index], entry);
          if (!conflict) return false;
          if (!klr_conflict_create_and_add_item(&collec->pool.itempool, conflict, rule, rule->bodylen) ||
              (entry->action == KLR_ACTION_RED &&
              !klr_conflict_create_and_add_item(&collec->pool.itempool, conflict, entry->info.rule, entry->info.rule->bodylen))) {
            klr_conflict_delete(&collec->pool, conflict);
            return false;
          }
          entry->action = KLR_ACTION_CON;
          entry->info.conflict = conflict;
          if (!conf_handler || !klr_conflict_handle(conf_handler, conflict, collec))
            klr_table_add_conflict(table, conflict);
          else
            klr_conflict_delete(&collec->pool, conflict);
        } else {
          entry->action = rule == start_rule ? KLR_ACTION_ACC : KLR_ACTION_RED;
          entry->info.rule = rule;
        }
        next_symbol_index = kbitset_iter_next(la, symbol_index);
      } while (next_symbol_index != symbol_index);
    }
  }
  return true;
}

static bool klr_init_transition_and_shifting_action(KlrTable* table, KlrCollection* collec) {
  KlrItemSet** itemsets = collec->itemsets;
  size_t nitemset = klr_collection_nitemset(collec);
  KlrTableEntry** entries = table->entries;
  for (size_t i = 0; i < nitemset; ++i) {
    KlrItemSet* itemset = itemsets[i];
    for (KlrItemSetTransition* trans = itemset->trans; trans; trans = trans->next) {
      size_t symbol_id = trans->symbol->id;
      size_t itemset_id = trans->target->id;
      entries[i][symbol_id].trans = itemset_id;
      if (klr_symbol_get_kind(trans->symbol) == KLR_TERMINAL) {
        entries[i][symbol_id].action = KLR_ACTION_SHI;
        entries[i][symbol_id].info.itemset_id = itemset_id;
      }
    }
  }
  return true;
}

KlrConflict* klr_conflict_create(KlrItemSetPool* pool, KlrItemSet* itemset, KlrSymbol* symbol, KlrTableEntry* entry) {
  KlrConflict* conflict = (KlrConflict*)malloc(sizeof (KlrConflict));
  if (!conflict) return NULL;
  conflict->next = NULL;
  conflict->itemset = itemset;
  conflict->symbol = symbol;
  conflict->entry = entry;
  conflict->conflict_items = klr_itemset_create(pool);
  if (!conflict->conflict_items) {
    free(conflict);
    return NULL;
  }
  return conflict;
}

void klr_conflict_delete(KlrItemPoolCollec* pool, KlrConflict* conflict) {
  if (!conflict) return;
  klr_itemset_delete(pool, conflict->conflict_items);
  free(conflict);
}

void klr_table_delete(KlrTable* table) {
  if (!table) return;
  if (table->entries) free(table->entries[0]);
  free(table->entries);
  KlrConflict* conflict = table->conflicts;
  while (conflict) {
    KlrConflict* tmp = conflict->next;
    klr_conflict_delete(&table->pool, conflict);
    conflict = tmp;
  }
  klr_itempoolcollec_destroy(&table->pool);
  free(table);
}

static inline void klr_table_add_conflict(KlrTable* table, KlrConflict* conflict) {
  conflict->next = table->conflicts;
  table->conflicts = conflict;
}

static KlrTableEntry** klr_table_get_initial_entries(size_t nsymbol, size_t nitemset) {
  KlrTableEntry** table = (KlrTableEntry**)malloc(sizeof (KlrTableEntry*) * nitemset);
  if (!table) return NULL;
  KlrTableEntry* entries = (KlrTableEntry*)malloc(sizeof (KlrTableEntry) * nitemset * nsymbol);
  if (!entries) {
    free(table);
    return NULL;
  }
  for (size_t i = 0; i < nitemset * nsymbol; ++i) {
    entries[i].trans = KLR_GOTO_NONE;
    entries[i].action = KLR_ACTION_ERR;
  }
  table[0] = entries;
  for (size_t i = 1; i < nitemset; ++i)
    table[i] = table[i - 1] + nsymbol;
  return table;
}

static bool klr_conflict_create_and_add_item(KlrItemPool* pool, KlrConflict* conflict, KlrRule* rule, size_t dot) {
  KlrItem* conflict_item = klr_item_create(pool, rule, dot);
  if (!conflict_item) return false;
  klr_conflict_add_item(conflict, conflict_item);
  return true;
}

KlrConflictHandler* klr_conflict_handler_create(void* object, KlrConflictCallback* callback) {
  KlrConflictHandler* handler = (KlrConflictHandler*)malloc(sizeof (KlrConflictHandler));
  if (!handler) return NULL;
  handler->object = object;
  handler->callback = callback;
  return handler;
}

void klr_conflict_handler_delete(KlrConflictHandler* handler) {
  free(handler);
}
