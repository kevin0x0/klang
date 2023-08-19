#include "pargen/include/lr/lr.h"

#include <stdlib.h>

static bool kev_lr_decide_action(KevItemSet* itemset, KevLRCollection* collec, KevLRGoto* goto_table, KevLRAction* action_table, KevAddrArray* closure, KevBitSet** la_symbols);

KevLRAction* kev_lr_action_create(KevLRCollection* collec, KevLRGoto* goto_table) {
  size_t symbol_no = collec->symbol_no;
  size_t terminal_no = collec->terminal_no;
  size_t itemset_no = collec->itemset_no;
  KevItemSet** itemsets = collec->itemsets;
  KevBitSet** la_symbols = (KevBitSet**)malloc(sizeof (KevBitSet*) * symbol_no);
  KevAddrArray* closure = kev_addrarray_create();
  if (!la_symbols || !closure) {
    free(la_symbols);
    kev_addrarray_delete(closure);
    return NULL;
  }
  for (size_t i = 0; i < symbol_no; ++i)
    la_symbols[i] = NULL;

  KevLRAction* ret_table = (KevLRAction*)malloc(sizeof (KevLRAction));
  if (!ret_table) {
    kev_lr_closure_delete(closure, la_symbols);
    return NULL;
  }
  ret_table->itemset_no = itemset_no;
  ret_table->symbol_no = symbol_no;
  ret_table->conflicts = NULL;
  KevLRActionEntry** table = (KevLRActionEntry**)malloc(sizeof (KevLRActionEntry*) * itemset_no);
  if (!table) {
    free(ret_table);
    kev_lr_closure_delete(closure, la_symbols);
    return NULL;
  }
  ret_table->table = table;
  KevLRActionEntry* tmp = (KevLRActionEntry*)malloc(sizeof (KevLRActionEntry) * itemset_no * terminal_no);
  if (!tmp) {
    free(ret_table->table);
    free(ret_table);
    kev_lr_closure_delete(closure, la_symbols);
    return NULL;
  }
  for (size_t i = 0; i < itemset_no * terminal_no; ++i)
    tmp[i].action = KEV_LR_ACTION_ERR;

  table[0] = tmp;
  for (size_t i = 1; i < itemset_no; ++i)
    table[i] = table[i - 1] + symbol_no;

  for (size_t i = 0; i < itemset_no; ++i) {
    if (!kev_lr_closure(itemsets[i], closure, la_symbols, collec->firsts, collec->terminal_no) ||
        !kev_lr_decide_action(itemsets[i], collec, goto_table, ret_table, closure, la_symbols)) {
      kev_lr_action_delete(ret_table);
      kev_lr_closure_delete(closure, la_symbols);
      return NULL;
    }
  }
  return ret_table;
}

static bool kev_lr_decide_action(KevItemSet* itemset, KevLRCollection* collec, KevLRGoto* goto_table, KevLRAction* action_table, KevAddrArray* closure, KevBitSet** la_symbols) {
  size_t itemset_id = itemset->id;
  KevLRActionEntry** table = action_table->table;
  KevLRGotoEntry** goto_tab = goto_table->table;
  KevSymbol** symbols = collec->symbols;
  KevRule* start_rule = collec->start_rule;
  /* for kernel item */
  for (KevItem* kitem = itemset->items; kitem; kitem = kitem->next) {
    KevRule* rule = kitem->rule;
    if (kitem->dot == rule->bodylen) {  /* reduce */
      KevBitSet* la = kitem->lookahead;
      /* lookahead is guaranteed to be not empty */
      size_t next_tmp_id = kev_bitset_iterate_begin(la);
      size_t tmp_id = 0;
      do {
        tmp_id = next_tmp_id;
        size_t id = symbols[tmp_id]->id;
        if (table[itemset_id][id].action != KEV_LR_ACTION_ERR) {
          KevLRConflict* conflict = NULL;
          if (table[itemset_id][id].action == KEV_LR_ACTION_SHI)
            conflict = kev_lr_conflict_create(itemset, collec->symbols[tmp_id], KEV_LR_CONFLICT_SR);
          else
            conflict = kev_lr_conflict_create(itemset, collec->symbols[tmp_id], KEV_LR_CONFLICT_RR);
          if (!conflict) return NULL;
          kev_lr_action_add_conflict(action_table, conflict);
        } else {
          if (rule == start_rule) {
            table[itemset_id][id].action = KEV_LR_ACTION_ACC;
          } else {
            table[itemset_id][id].action = KEV_LR_ACTION_RED;
            table[itemset_id][id].info.rule = rule;
          }
        }
        next_tmp_id = kev_bitset_iterate_next(la, tmp_id);
      } while (next_tmp_id != tmp_id);
    } else {  /* shift */
      size_t id = rule->body[kitem->dot]->id;
      if (table[itemset_id][id].action != KEV_LR_ACTION_SHI) {
        KevLRConflict* conflict = kev_lr_conflict_create(itemset, rule->body[kitem->dot], KEV_LR_CONFLICT_SR);
        if (!conflict) return NULL;
        kev_lr_action_add_conflict(action_table, conflict);
      } else {
        table[itemset_id][id].action = KEV_LR_ACTION_SHI;
        table[itemset_id][id].info.itemset = goto_tab[itemset_id][id];
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
          if (table[itemset_id][id].action != KEV_LR_ACTION_ERR) {
            KevLRConflict* conflict = NULL;
            if (table[itemset_id][id].action == KEV_LR_ACTION_SHI)
              conflict = kev_lr_conflict_create(itemset, collec->symbols[tmp_id], KEV_LR_CONFLICT_SR);
            else
              conflict = kev_lr_conflict_create(itemset, collec->symbols[tmp_id], KEV_LR_CONFLICT_RR);
            if (!conflict) return NULL;
            kev_lr_action_add_conflict(action_table, conflict);
          } else {
            /* item that contains starting rule is kernel item, so it is OK
             * to not check whether the rule is starting rule. */
            table[itemset_id][id].action = KEV_LR_ACTION_RED;
            table[itemset_id][id].info.rule = rule;
          }
          next_tmp_id = kev_bitset_iterate_next(la, tmp_id);
        } while (next_tmp_id != tmp_id);
      } else {  /* shift */
        size_t id = rule->body[0]->id;
        if (table[itemset_id][id].action != KEV_LR_ACTION_SHI) {
          KevLRConflict* conflict = kev_lr_conflict_create(itemset, rule->body[0], KEV_LR_CONFLICT_SR);
            if (!conflict) return NULL;
            kev_lr_action_add_conflict(action_table, conflict);
        } else {
          table[itemset_id][id].action = KEV_LR_ACTION_SHI;
          table[itemset_id][id].info.itemset = goto_tab[itemset_id][id];
        }
      }
    }
  }
  return true;
}

KevLRGoto* kev_lr_goto_create(KevLRCollection* collec) {
  KevItemSet** itemsets = collec->itemsets;
  size_t itemset_no = collec->itemset_no;
  size_t symbol_no = collec->symbol_no;
  KevLRGoto* ret_table = (KevLRGoto*)malloc(sizeof (KevLRGoto));
  if (!ret_table) return NULL;
  ret_table->symbol_no = symbol_no;
  ret_table->itemset_no = itemset_no;
  ret_table->table = (KevLRGotoEntry**)malloc(sizeof (KevLRGotoEntry) * itemset_no);
  if (!ret_table->table) {
    free(ret_table);
    return NULL;
  }
  KevLRGotoEntry* tmp = (KevLRGotoEntry*)malloc(sizeof (KevLRGotoEntry) * itemset_no * symbol_no);
  if (!tmp) {
    free(ret_table->table);
    free(ret_table);
    return NULL;
  }
  for (size_t i = 0; i < itemset_no * symbol_no; ++i)
    tmp[i] = KEV_LR_GOTO_NONE;

  KevLRGotoEntry** table = ret_table->table;
  table[0] = tmp;
  for (size_t i = 1; i < itemset_no; ++i)
    table[i] = table[i - 1] + symbol_no;

  for (size_t i = 0; i < itemset_no; ++i) {
    KevItemSet* itemset = itemsets[i];
    for (KevItemSetGoto* gotos = itemset->gotos; gotos; gotos = gotos->next)
      table[i][gotos->symbol->id] = gotos->itemset->id;
  }
  return ret_table;
}

void kev_lr_action_delete(KevLRAction* table) {
  if (!table) return;
  free(table->table[0]);
  free(table->table);
  free(table);
}

void kev_lr_goto_delete(KevLRGoto* table) {
  if (!table) return;
  free(table->table[0]);
  free(table->table);
  free(table);
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
