#include "pargen/include/lr/lr.h"

#include <stdlib.h>

static bool kev_lr_decide_action(KevItemSet* itemset, KevLRCollection* collec, KevLRGoto* goto_table, KevBitSet** la_symbols, KevAddrArray* closure);

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
    return ret_table;
  }
  KevLRActionEntry** table = (KevLRActionEntry**)malloc(sizeof (KevLRActionEntry*) * itemset_no);
  if (!table) {
    free(ret_table);
    kev_lr_closure_delete(closure, la_symbols);
    return ret_table;
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
        !kev_lr_decide_action(itemsets[i], collec, goto_table, la_symbols, closure)) {
      kev_lr_action_delete(ret_table);
      kev_lr_closure_delete(closure, la_symbols);
      return NULL;
    }
  }
  return ret_table;
}

static bool kev_lr_decide_action(KevItemSet* itemset, KevLRCollection* collec, KevBitSet** la_symbols, KevAddrArray* closure) {

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
