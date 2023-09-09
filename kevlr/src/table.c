#include "kevlr/include/table.h"
#include "kevlr/include/lr_utils.h"

#include <stdlib.h>

static bool kev_lr_decide_action(KevLRCollection* collec, KevLRTable* table, KevItemSet* itemset,
                                 KevItemSetClosure* closure, KevLRConflictHandler* conf_handler);
static bool kev_lr_init_reducing_action(KevLRTable* table, KevLRCollection* collec, KevLRConflictHandler* conf_handler);
static bool kev_lr_init_goto_and_shifting_action(KevLRTable* table, KevLRCollection* collec);
static inline void kev_lr_table_add_conflict(KevLRTable* table, KevLRConflict* conflict);
static KevLRTableEntry** kev_lr_table_get_initial_entries(size_t symbol_no, size_t itemset_no);
static bool kev_lr_conflict_create_and_add_item(KevLRConflict* conflict, KevRule* rule, size_t dot);

KevLRTable* kev_lr_table_create(KevLRCollection* collec, KevLRConflictHandler* conf_handler) {
  KevLRTable* table = (KevLRTable*)malloc(sizeof (KevLRTable));
  if (!table) return NULL;
  /* start symbol is excluded in the table, so the actual symbol number for
   * the table is table->symbol_no - 1. */
  table->table_symbol_no = kev_lr_util_symbol_max_id(collec) + 1;
  table->total_symbol_no = kev_lr_collection_get_symbol_no(collec);
  table->terminal_no = kev_lr_collection_get_terminal_no(collec);
  table->itemset_no = kev_lr_collection_get_itemset_no(collec);
  table->entries = kev_lr_table_get_initial_entries(table->table_symbol_no, table->itemset_no);
  table->conflicts = NULL;
  if (!table->entries) {
    kev_lr_table_delete(table);
    return NULL;
  }

  if (!kev_lr_init_goto_and_shifting_action(table, collec) ||
      !kev_lr_init_reducing_action(table, collec, conf_handler)) {
    kev_lr_table_delete(table);
    return NULL;
  }
  return table;
}

static bool kev_lr_init_reducing_action(KevLRTable* table, KevLRCollection* collec, KevLRConflictHandler* conf_handler) {
  size_t total_symbol_no = table->total_symbol_no;
  size_t terminal_no = table->terminal_no;
  size_t itemset_no = table->itemset_no;
  KevItemSet** itemsets = collec->itemsets;
  KevItemSetClosure* closure = kev_lr_closure_create(total_symbol_no);
  if (!closure) return false;

  for (size_t i = 0; i < itemset_no; ++i) {
    if (!kev_lr_closure_make(closure, itemsets[i], collec->firsts, terminal_no) ||
        !kev_lr_decide_action(collec, table, itemsets[i], closure, conf_handler)) {
      kev_lr_closure_delete(closure);
      return false;
    }
    kev_lr_closure_make_empty(closure);
  }
  kev_lr_closure_delete(closure);
  return true;
}

static bool kev_lr_decide_action(KevLRCollection* collec, KevLRTable* table, KevItemSet* itemset,
                                 KevItemSetClosure* closure, KevLRConflictHandler* conf_handler) {
  size_t itemset_id = itemset->id;
  KevLRTableEntry** entries = table->entries;
  KevSymbol** symbols = kev_lr_collection_get_symbols(collec);
  KevRule* start_rule = collec->start_rule;
  KevAddrArray* closure_symbols = closure->symbols;
  KevBitSet** las = closure->lookaheads;
  /* for kernel item */
  for (KevItem* kitem = itemset->items; kitem; kitem = kitem->next) {
    KevRule* rule = kitem->rule;
    if (kitem->dot != rule->bodylen) continue;
    /* reduce */
    KevBitSet* la = kitem->lookahead;
    /* lookahead set is guaranteed to be not empty */
    size_t next_symbol_index = kev_bitset_iterate_begin(la);
    size_t symbol_index = 0;
    do {
      symbol_index = next_symbol_index;
      size_t id = symbols[symbol_index]->id;
      KevLRTableEntry* entry = &entries[itemset_id][id];
      if (entry->action == KEV_LR_ACTION_CON) {
        if (!kev_lr_conflict_create_and_add_item(entry->info.conflict, kitem->rule, kitem->dot))
          return false;
        continue;
      } else if (entry->action != KEV_LR_ACTION_ERR) {
        KevLRConflict* conflict = kev_lr_conflict_create(itemset, collec->symbols[symbol_index], entry);
        entry->action = KEV_LR_ACTION_CON;
        entry->info.conflict = conflict;
        if (!conflict) return false;
        if (!kev_lr_conflict_create_and_add_item(conflict, kitem->rule, kitem->dot) ||
            (entry->action == KEV_LR_ACTION_RED &&
            !kev_lr_conflict_create_and_add_item(conflict, entry->info.rule, entry->info.rule->bodylen))) {
          kev_lr_conflict_delete(conflict);
          return false;
        }
        if (!conf_handler || !kev_lr_conflict_handle(conf_handler, conflict, collec))
          kev_lr_table_add_conflict(table, conflict);
        else
          kev_lr_conflict_delete(conflict);
      } else {
        entry->action = rule == start_rule ? KEV_LR_ACTION_ACC : KEV_LR_ACTION_RED;
        entry->info.rule = rule;
      }
      next_symbol_index = kev_bitset_iterate_next(la, symbol_index);
    } while (next_symbol_index != symbol_index);
  }
  /* for non-kernel item */
  size_t closure_size = kev_addrarray_size(closure_symbols);
  for (size_t i = 0; i < closure_size; ++i) {
    KevSymbol* head = (KevSymbol*)kev_addrarray_visit(closure_symbols, i);
    KevBitSet* la = las[head->index];
    for (KevRuleNode* node = head->rules; node; node = node->next) {
      KevRule* rule = node->rule;
      if (rule->bodylen == 0) {  /* reduce */
      /* lookahead is guaranteed to be not empty. */
        size_t symbol_index = kev_bitset_iterate_begin(la);
        size_t next_symbol_index = 0;
        do {
          symbol_index = next_symbol_index;
          size_t id = symbols[symbol_index]->id;
          KevLRTableEntry* entry = &entries[itemset_id][id];
          if (entry->action == KEV_LR_ACTION_CON) {
            if (!kev_lr_conflict_create_and_add_item(entry->info.conflict, rule, rule->bodylen))
              return false;
            continue;
          } else if (entry->action != KEV_LR_ACTION_ERR) {
            KevLRConflict* conflict = kev_lr_conflict_create(itemset, collec->symbols[symbol_index], entry);
            entry->action = KEV_LR_ACTION_CON;
            entry->info.conflict = conflict;
            if (!conflict) return false;
            if (!kev_lr_conflict_create_and_add_item(conflict, rule, rule->bodylen) ||
                !(entry->action == KEV_LR_ACTION_RED &&
                kev_lr_conflict_create_and_add_item(conflict, entry->info.rule, entry->info.rule->bodylen))) {
              kev_lr_conflict_delete(conflict);
              return false;
            }
            if (!conf_handler || !kev_lr_conflict_handle(conf_handler, conflict, collec))
              kev_lr_table_add_conflict(table, conflict);
            else
              kev_lr_conflict_delete(conflict);
          } else {
            entry->action = rule == start_rule ? KEV_LR_ACTION_ACC : KEV_LR_ACTION_RED;
            entry->info.rule = rule;
          }
          next_symbol_index = kev_bitset_iterate_next(la, symbol_index);
        } while (next_symbol_index != symbol_index);
      }
    }
  }
  return true;
}

static bool kev_lr_init_goto_and_shifting_action(KevLRTable* table, KevLRCollection* collec) {
  KevItemSet** itemsets = collec->itemsets;
  size_t itemset_no = kev_lr_collection_get_itemset_no(collec);
  KevLRTableEntry** entries = table->entries;
  for (size_t i = 0; i < itemset_no; ++i) {
    KevItemSet* itemset = itemsets[i];
    for (KevItemSetGoto* goto_item = itemset->gotos; goto_item; goto_item = goto_item->next) {
      size_t symbol_id = goto_item->symbol->id;
      size_t itemset_id = goto_item->itemset->id;
      entries[i][symbol_id].go_to = itemset_id;
      entries[i][symbol_id].action = KEV_LR_ACTION_SHI;
      entries[i][symbol_id].info.itemset_id = itemset_id;
    }
  }
  return true;
}

KevLRConflict* kev_lr_conflict_create(KevItemSet* itemset, KevSymbol* symbol, KevLRTableEntry* entry) {
  KevLRConflict* conflict = (KevLRConflict*)malloc(sizeof (KevLRConflict));
  if (!conflict) return NULL;
  conflict->next = NULL;
  conflict->itemset = itemset;
  conflict->symbol = symbol;
  conflict->entry = entry;
  conflict->conflct_items = kev_lr_itemset_create();
  if (!conflict->conflct_items) {
    free(conflict);
    return NULL;
  }
  return conflict;
}

void kev_lr_conflict_delete(KevLRConflict* conflict) {
  if (!conflict) return;
  kev_lr_itemset_delete(conflict->conflct_items);
  free(conflict);
}

void kev_lr_table_delete(KevLRTable* table) {
  if (!table) return;
  free(table->entries[0]);
  free(table->entries);
  KevLRConflict* conflict = table->conflicts;
  while (conflict) {
    KevLRConflict* tmp = conflict->next;
    kev_lr_conflict_delete(conflict);
    conflict = tmp;
  }
  free(table);
}

static inline void kev_lr_table_add_conflict(KevLRTable* table, KevLRConflict* conflict) {
  conflict->next = table->conflicts;
  table->conflicts = conflict;
}

static KevLRTableEntry** kev_lr_table_get_initial_entries(size_t symbol_no, size_t itemset_no) {
  KevLRTableEntry** table = (KevLRTableEntry**)malloc(sizeof (KevLRTableEntry*) * itemset_no);
  if (!table) return NULL;
  KevLRTableEntry* entries = (KevLRTableEntry*)malloc(sizeof (KevLRTableEntry) * itemset_no * symbol_no);
  if (!entries) {
    free(table);
    return NULL;
  }
  for (size_t i = 0; i < itemset_no * symbol_no; ++i) {
    entries[i].go_to = KEV_LR_GOTO_NONE;
    entries[i].action = KEV_LR_ACTION_ERR;
  }
  table[0] = entries;
  for (size_t i = 1; i < itemset_no; ++i)
    table[i] = table[i - 1] + symbol_no;
  return table;
}

static bool kev_lr_conflict_create_and_add_item(KevLRConflict* conflict, KevRule* rule, size_t dot) {
  KevItem* conflict_item = kev_lr_item_create(rule, rule->bodylen);
  if (!conflict_item) return false;
  kev_lr_conflict_add_item(conflict, conflict_item);
  return true;
}

KevLRConflictHandler* kev_lr_conflict_handler_create(void* object, KevLRConflictCallback* callback) {
  KevLRConflictHandler* handler = (KevLRConflictHandler*)malloc(sizeof (KevLRConflictHandler));
  if (!handler) return NULL;
  handler->object = object;
  handler->callback = callback;
  return handler;
}

void kev_lr_conflict_handler_delete(KevLRConflictHandler* handler) {
  free(handler);
}