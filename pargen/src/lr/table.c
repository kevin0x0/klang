#include "pargen/include/lr/table.h"

#include <stdlib.h>

static bool kev_lr_decide_action(KevLRCollection* collec, KevLRTable* table, KevItemSet* itemset,
                                 KevItemSetClosure* closure, KevLRConflictHandler conf_handler);
static bool kev_lr_init_action(KevLRTable* table, KevLRCollection* collec, KevLRConflictHandler conf_handler);
static bool kev_lr_init_goto(KevLRTable* table, KevLRCollection* collec);
static inline void kev_lr_table_add_conflict(KevLRTable* table, KevLRConflict* conflict);
static KevLRTableEntry** kev_lr_table_get_initial_entries(size_t symbol_no, size_t itemset_no);


KevLRTable* kev_lr_table_create(KevLRCollection* collec, KevLRConflictHandler conf_handler) {
  KevLRTable* table = (KevLRTable*)malloc(sizeof (KevLRTable));
  if (!table) return NULL;
  table->symbol_no = collec->symbol_no;
  table->terminal_no = collec->terminal_no;
  table->itemset_no = collec->itemset_no;
  table->conflicts = NULL;
  table->entries = kev_lr_table_get_initial_entries(table->symbol_no, table->itemset_no);
  if (!table->entries) {
    kev_lr_table_delete(table);
    return NULL;
  }

  if (!kev_lr_init_goto(table, collec) ||
      !kev_lr_init_action(table, collec, conf_handler)) {
    kev_lr_table_delete(table);
    return NULL;
  }
  return table;
}

static bool kev_lr_init_action(KevLRTable* table, KevLRCollection* collec, KevLRConflictHandler conf_handler) {
  size_t symbol_no = table->symbol_no;
  size_t terminal_no = table->terminal_no;
  size_t itemset_no = table->itemset_no;
  KevItemSet** itemsets = collec->itemsets;
  KevItemSetClosure* closure = kev_lr_closure_create(symbol_no);
  if (!closure) return false;

  for (size_t i = 0; i < itemset_no; ++i) {
    if (!kev_lr_closure_make(closure, itemsets[i], collec->firsts, terminal_no) ||
        !kev_lr_decide_action(collec, table, itemsets[i], closure, conf_handler)) {
      kev_lr_closure_delete(closure);
      return false;
    }
  }
  kev_lr_closure_delete(closure);
  return true;
}

static bool kev_lr_decide_action(KevLRCollection* collec, KevLRTable* table, KevItemSet* itemset,
                                 KevItemSetClosure* closure, KevLRConflictHandler conf_handler) {
  size_t itemset_id = itemset->id;
  KevLRTableEntry** entries = table->entries;
  KevSymbol** symbols = collec->symbols;
  KevRule* start_rule = collec->start_rule;
  KevAddrArray* closure_symbols = closure->symbols;
  KevBitSet** las = closure->lookaheads;
  /* for kernel item */
  for (KevItem* kitem = itemset->items; kitem; kitem = kitem->next) {
    KevRule* rule = kitem->rule;
    if (kitem->dot == rule->bodylen) {  /* reduce */
      KevBitSet* la = kitem->lookahead;
      /* lookahead set is guaranteed to be not empty */
      size_t next_tmp_id = kev_bitset_iterate_begin(la);
      size_t tmp_id = 0;
      do {
        tmp_id = next_tmp_id;
        size_t id = symbols[tmp_id]->id;
        if (entries[itemset_id][id].action != KEV_LR_ACTION_ERR) {
          if (entries[itemset_id][id].action == KEV_LR_ACTION_CON)
            continue;
          KevLRConflict* conflict = NULL;
          if (entries[itemset_id][id].action == KEV_LR_ACTION_SHI)
            conflict = kev_lr_conflict_create(itemset, collec->symbols[tmp_id], closure, KEV_LR_CONFLICT_SR);
          else
            conflict = kev_lr_conflict_create(itemset, collec->symbols[tmp_id], closure, KEV_LR_CONFLICT_RR);
          if (!conflict) return NULL;
          if (!conf_handler || !conf_handler(conflict, collec)) {
            kev_lr_table_add_conflict(table, conflict);
            entries[itemset_id][id].action = KEV_LR_ACTION_CON;
            entries[itemset_id][id].info.conflict = conflict;
          }
        } else {
          if (rule == start_rule) {
            entries[itemset_id][id].action = KEV_LR_ACTION_ACC;
          } else {
            entries[itemset_id][id].action = KEV_LR_ACTION_RED;
            entries[itemset_id][id].info.rule = rule;
          }
        }
        next_tmp_id = kev_bitset_iterate_next(la, tmp_id);
      } while (next_tmp_id != tmp_id);
    } else {  /* shift */
      size_t id = rule->body[kitem->dot]->id;
      if (entries[itemset_id][id].action != KEV_LR_ACTION_SHI) {
        if (entries[itemset_id][id].action != KEV_LR_ACTION_CON) {
          KevLRConflict* conflict = kev_lr_conflict_create(itemset, rule->body[kitem->dot], closure, KEV_LR_CONFLICT_SR);
          if (!conflict) return NULL;
          kev_lr_table_add_conflict(table, conflict);
          entries[itemset_id][id].action = KEV_LR_ACTION_CON;
          entries[itemset_id][id].info.conflict = conflict;
        }
      } else {
        entries[itemset_id][id].action = KEV_LR_ACTION_SHI;
        entries[itemset_id][id].info.itemset = entries[itemset_id][id].go_to;
      }
    }
  }
  /* for non-kernel item */
  size_t closure_size = kev_addrarray_size(closure_symbols);
  for (size_t i = 0; i < closure_size; ++i) {
    KevSymbol* head = kev_addrarray_visit(closure_symbols, i);
    KevBitSet* la = las[head->tmp_id];
    for (KevRuleNode* node = head->rules; node; node = node->next) {
      KevRule* rule = node->rule;
      if (rule->bodylen == 0) {  /* reduce */
      /* lookahead is guaranteed to be not empty. */
        size_t next_tmp_id = kev_bitset_iterate_begin(la);
        size_t tmp_id = 0;
        do {
          tmp_id = next_tmp_id;
          size_t id = symbols[tmp_id]->id;
          if (entries[itemset_id][id].action != KEV_LR_ACTION_ERR) {
            if (entries[itemset_id][id].action == KEV_LR_ACTION_CON)
              continue;
            KevLRConflict* conflict = NULL;
            if (entries[itemset_id][id].action == KEV_LR_ACTION_SHI)
              conflict = kev_lr_conflict_create(itemset, collec->symbols[tmp_id], closure, KEV_LR_CONFLICT_SR);
            else
              conflict = kev_lr_conflict_create(itemset, collec->symbols[tmp_id], closure, KEV_LR_CONFLICT_RR);
            if (!conflict) return NULL;
            kev_lr_table_add_conflict(table, conflict);
            entries[itemset_id][id].action = KEV_LR_ACTION_CON;
            entries[itemset_id][id].info.conflict = conflict;
          } else {
            /* Item that contains starting rule is kernel item, so it is OK
             * to not check whether the rule is starting rule. */
            entries[itemset_id][id].action = KEV_LR_ACTION_RED;
            entries[itemset_id][id].info.rule = rule;
          }
          next_tmp_id = kev_bitset_iterate_next(la, tmp_id);
        } while (next_tmp_id != tmp_id);
      } else {  /* shift */
        size_t id = rule->body[0]->id;
        if (entries[itemset_id][id].action != KEV_LR_ACTION_SHI) {
          if (entries[itemset_id][id].action != KEV_LR_ACTION_CON) {
            KevLRConflict* conflict = kev_lr_conflict_create(itemset, rule->body[0], closure, KEV_LR_CONFLICT_SR);
            if (!conflict) return NULL;
            kev_lr_table_add_conflict(table, conflict);
            entries[itemset_id][id].action = KEV_LR_ACTION_CON;
            entries[itemset_id][id].info.conflict = conflict;
          }
        } else {
          entries[itemset_id][id].action = KEV_LR_ACTION_SHI;
          entries[itemset_id][id].info.itemset = entries[itemset_id][id].go_to;
        }
      }
    }
  }
  return true;
}

static bool kev_lr_init_goto(KevLRTable* table, KevLRCollection* collec) {
  KevItemSet** itemsets = collec->itemsets;
  size_t itemset_no = collec->itemset_no;
  KevLRTableEntry** entries = table->entries;
  for (size_t i = 0; i < itemset_no; ++i) {
    KevItemSet* itemset = itemsets[i];
    for (KevItemSetGoto* goto_item = itemset->gotos; goto_item; goto_item = goto_item->next)
      entries[i][goto_item->symbol->id].go_to = goto_item->itemset->id;
  }
  return true;
}

KevLRConflict* kev_lr_conflict_create(KevItemSet* itemset, KevSymbol* symbol, KevItemSetClosure* closure, int conflict_type) {
  KevLRConflict* conflict = (KevLRConflict*)malloc(sizeof (KevLRConflict));
  if (!conflict) return NULL;
  conflict->next = NULL;
  conflict->conflict_itemset = itemset;
  conflict->symbol = symbol;
  conflict->closure = closure;
  conflict->conflict_type = conflict_type;
  return conflict;
}

void kev_lr_conflict_delete(KevLRConflict* conflict) {
  if (!conflict) return;
  free(conflict);
}

void kev_lr_table_delete(KevLRTable* table) {
  if (!table) return;
  free(table->entries[0]);
  free(table->entries);
  KevLRConflict* conflict = table->conflicts;
  while (conflict) {
    KevLRConflict* tmp = conflict->next;
    free(conflict);
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

