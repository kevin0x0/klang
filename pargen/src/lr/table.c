#include "pargen/include/lr/table.h"

#include <stdlib.h>

static bool kev_lr_decide_action(KevItemSet* itemset, KevLRCollection* collec,
                                 KevLRTable* table, KevAddrArray* closure, KevBitSet** la_symbols);
static bool kev_lr_action_create(KevLRTable* table, KevLRCollection* collec);
static KevLRGotoEntry** kev_lr_goto_create(KevLRCollection* collec);
static inline void kev_lr_table_add_conflict(KevLRTable* table, KevLRConflict* conflict);


KevLRTable* kev_lr_table_create(KevLRCollection* collec) {
  KevLRTable* table = (KevLRTable*)malloc(sizeof (KevLRTable));
  if (!table) return NULL;
  table->gotos = NULL;
  table->actions = NULL;
  table->conflicts = NULL;
  table->symbol_no = collec->symbol_no;
  table->terminal_no = collec->terminal_no;
  table->itemset_no = collec->itemset_no;

  table->gotos = kev_lr_goto_create(collec);
  if (!table->gotos) {
    kev_lr_table_delete(table);
    return NULL;
  }
  if (!kev_lr_action_create(table, collec)) {
    kev_lr_table_delete(table);
    return NULL;
  }
  return table;
}

static bool kev_lr_action_create(KevLRTable* table, KevLRCollection* collec) {
  size_t symbol_no = table->symbol_no;
  size_t terminal_no = table->terminal_no;
  size_t itemset_no = table->itemset_no;
  KevItemSet** itemsets = collec->itemsets;
  KevBitSet** la_symbols = (KevBitSet**)malloc(sizeof (KevBitSet*) * symbol_no);
  KevAddrArray* closure = kev_addrarray_create();
  if (!la_symbols || !closure) {
    free(la_symbols);
    kev_addrarray_delete(closure);
    return false;
  }
  for (size_t i = 0; i < symbol_no; ++i)
    la_symbols[i] = NULL;

  KevLRActionEntry** actions = (KevLRActionEntry**)malloc(sizeof (KevLRActionEntry*) * itemset_no);
  if (!actions) {
    kev_lr_closure_delete(closure, la_symbols);
    return false;
  }
  KevLRActionEntry* tmp = (KevLRActionEntry*)malloc(sizeof (KevLRActionEntry) * itemset_no * symbol_no);
  if (!tmp) {
    free(actions);
    kev_lr_closure_delete(closure, la_symbols);
    return false;
  }
  for (size_t i = 0; i < itemset_no * symbol_no; ++i)
    tmp[i].action = KEV_LR_ACTION_ERR;

  actions[0] = tmp;
  for (size_t i = 1; i < itemset_no; ++i)
    actions[i] = actions[i - 1] + symbol_no;
  table->actions = actions;

  for (size_t i = 0; i < itemset_no; ++i) {
    if (!kev_lr_closure(itemsets[i], closure, la_symbols, collec->firsts, terminal_no) ||
        !kev_lr_decide_action(itemsets[i], collec, table, closure, la_symbols)) {
      free(tmp);
      free(actions);
      kev_lr_closure_delete(closure, la_symbols);
      return false;
    }
  }
  kev_lr_closure_delete(closure, la_symbols);
  return true;
}

static bool kev_lr_decide_action(KevItemSet* itemset, KevLRCollection* collec,
                                 KevLRTable* table, KevAddrArray* closure, KevBitSet** la_symbols) {
  size_t itemset_id = itemset->id;
  KevLRActionEntry** actions = table->actions;
  KevLRGotoEntry** gotos = table->gotos;
  KevSymbol** symbols = collec->symbols;
  KevRule* start_rule = collec->start_rule;
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
        if (actions[itemset_id][id].action != KEV_LR_ACTION_ERR) {
          if (actions[itemset_id][id].action == KEV_LR_ACTION_CON)
            continue;
          KevLRConflict* conflict = NULL;
          if (actions[itemset_id][id].action == KEV_LR_ACTION_SHI)
            conflict = kev_lr_conflict_create(itemset, collec->symbols[tmp_id], KEV_LR_CONFLICT_SR);
          else
            conflict = kev_lr_conflict_create(itemset, collec->symbols[tmp_id], KEV_LR_CONFLICT_RR);
          if (!conflict) return NULL;
          kev_lr_table_add_conflict(table, conflict);
          actions[itemset_id][id].action = KEV_LR_ACTION_CON;
          actions[itemset_id][id].info.conflict = conflict;
        } else {
          if (rule == start_rule) {
            actions[itemset_id][id].action = KEV_LR_ACTION_ACC;
          } else {
            actions[itemset_id][id].action = KEV_LR_ACTION_RED;
            actions[itemset_id][id].info.rule = rule;
          }
        }
        next_tmp_id = kev_bitset_iterate_next(la, tmp_id);
      } while (next_tmp_id != tmp_id);
    } else {  /* shift */
      size_t id = rule->body[kitem->dot]->id;
      if (actions[itemset_id][id].action != KEV_LR_ACTION_SHI) {
        if (actions[itemset_id][id].action != KEV_LR_ACTION_CON) {
          KevLRConflict* conflict = kev_lr_conflict_create(itemset, rule->body[kitem->dot], KEV_LR_CONFLICT_SR);
          if (!conflict) return NULL;
          kev_lr_table_add_conflict(table, conflict);
          actions[itemset_id][id].action = KEV_LR_ACTION_CON;
          actions[itemset_id][id].info.conflict = conflict;
        }
      } else {
        actions[itemset_id][id].action = KEV_LR_ACTION_SHI;
        actions[itemset_id][id].info.itemset = gotos[itemset_id][id];
      }
    }
  }
  /* for non-kernel item */
  size_t closure_size = kev_addrarray_size(closure);
  for (size_t i = 0; i < closure_size; ++i) {
    KevSymbol* head = kev_addrarray_visit(closure, i);
    KevBitSet* la = la_symbols[head->tmp_id];
    for (KevRuleNode* node = head->rules; node; node = node->next) {
      KevRule* rule = node->rule;
      if (rule->bodylen == 0) {  /* reduce */
      /* lookahead is guaranteed to be not empty. */
        size_t next_tmp_id = kev_bitset_iterate_begin(la);
        size_t tmp_id = 0;
        do {
          tmp_id = next_tmp_id;
          size_t id = symbols[tmp_id]->id;
          if (actions[itemset_id][id].action != KEV_LR_ACTION_ERR) {
            if (actions[itemset_id][id].action == KEV_LR_ACTION_CON)
              continue;
            KevLRConflict* conflict = NULL;
            if (actions[itemset_id][id].action == KEV_LR_ACTION_SHI)
              conflict = kev_lr_conflict_create(itemset, collec->symbols[tmp_id], KEV_LR_CONFLICT_SR);
            else
              conflict = kev_lr_conflict_create(itemset, collec->symbols[tmp_id], KEV_LR_CONFLICT_RR);
            if (!conflict) return NULL;
            kev_lr_table_add_conflict(table, conflict);
            actions[itemset_id][id].action = KEV_LR_ACTION_CON;
            actions[itemset_id][id].info.conflict = conflict;
          } else {
            /* Item that contains starting rule is kernel item, so it is OK
             * to not check whether the rule is starting rule. */
            actions[itemset_id][id].action = KEV_LR_ACTION_RED;
            actions[itemset_id][id].info.rule = rule;
          }
          next_tmp_id = kev_bitset_iterate_next(la, tmp_id);
        } while (next_tmp_id != tmp_id);
      } else {  /* shift */
        size_t id = rule->body[0]->id;
        if (actions[itemset_id][id].action != KEV_LR_ACTION_SHI) {
          if (actions[itemset_id][id].action != KEV_LR_ACTION_CON) {
            KevLRConflict* conflict = kev_lr_conflict_create(itemset, rule->body[0], KEV_LR_CONFLICT_SR);
            if (!conflict) return NULL;
            kev_lr_table_add_conflict(table, conflict);
            actions[itemset_id][id].action = KEV_LR_ACTION_CON;
            actions[itemset_id][id].info.conflict = conflict;
          }
        } else {
          actions[itemset_id][id].action = KEV_LR_ACTION_SHI;
          actions[itemset_id][id].info.itemset = gotos[itemset_id][id];
        }
      }
    }
  }
  return true;
}

static KevLRGotoEntry** kev_lr_goto_create(KevLRCollection* collec) {
  KevItemSet** itemsets = collec->itemsets;
  size_t itemset_no = collec->itemset_no;
  size_t symbol_no = collec->symbol_no;
  KevLRGotoEntry** gotos = (KevLRGotoEntry**)malloc(sizeof (KevLRGotoEntry) * itemset_no);
  if (!gotos) return NULL;
  KevLRGotoEntry* tmp = (KevLRGotoEntry*)malloc(sizeof (KevLRGotoEntry) * itemset_no * symbol_no);
  if (!tmp) {
    free(gotos);
    return NULL;
  }
  for (size_t i = 0; i < itemset_no * symbol_no; ++i)
    tmp[i] = KEV_LR_GOTO_NONE;

  gotos[0] = tmp;
  for (size_t i = 1; i < itemset_no; ++i)
    gotos[i] = gotos[i - 1] + symbol_no;

  for (size_t i = 0; i < itemset_no; ++i) {
    KevItemSet* itemset = itemsets[i];
    for (KevItemSetGoto* goto_item = itemset->gotos; goto_item; goto_item = goto_item->next)
      gotos[i][goto_item->symbol->id] = goto_item->itemset->id;
  }
  return gotos;
}

KevLRConflict* kev_lr_conflict_create(KevItemSet* itemset, KevSymbol* symbol, int conflict_type) {
  KevLRConflict* conflict = (KevLRConflict*)malloc(sizeof (KevLRConflict));
  if (!conflict) return NULL;
  conflict->next = NULL;
  conflict->conflict_itemset = itemset;
  conflict->symbol = symbol;
  conflict->type = conflict_type;
  return conflict;
}

void kev_lr_table_delete(KevLRTable* table) {
  if (!table) return;
  free(table->gotos[0]);
  free(table->gotos);
  free(table->actions[0]);
  free(table->actions);
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
