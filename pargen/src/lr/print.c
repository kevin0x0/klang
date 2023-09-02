#include "pargen/include/lr/print.h"

#include <stdlib.h>
#include <string.h>

#define KEV_LR_SYMBOL_EPSILON_STRING  "ε"
#define KEV_LR_DOT                    "·"
#define KEV_LR_SYMBOL_UNNAMED         "[UNNAMED]"

static inline size_t kev_numlen(size_t num);
static inline size_t kev_max(size_t num1, size_t num2);
static int kev_symbol_compare(const void* sym1, const void* sym2);


static void kev_lr_print_itemset_with_closure(FILE* out, KevLRCollection* collec, KevItemSet* itemset, KevItemSetClosure* closure);

static inline size_t kev_numlen(size_t num) {
  char numstr[sizeof (size_t) * 4];
  return sprintf(numstr, "%llu", num);
}

static inline size_t kev_max(size_t num1, size_t num2) {
  return num1 > num2 ? num1 : num2;
}

static int kev_symbol_compare(const void* sym1, const void* sym2) {
  return (*(KevSymbol**)sym1)->id - (*(KevSymbol**)sym2)->id;
}

static void kev_lr_print_itemset_with_closure(FILE* out, KevLRCollection* collec, KevItemSet* itemset, KevItemSetClosure* closure) {
  for (KevItem* kitem = itemset->items; kitem; kitem = kitem->next) {
    kev_lr_print_kernel_item(out, collec, kitem);
    fputc('\n', out);
  }
  if (!closure) return;
  KevAddrArray* symbols = closure->symbols;
  KevBitSet** las = closure->lookaheads;
  for (size_t i = 0; i < kev_addrarray_size(symbols); ++i) {
    KevSymbol* head = kev_addrarray_visit(symbols, i);
    size_t head_index = head->tmp_id;
    for (KevRuleNode* node = head->rules; node; node = node->next) {
      KevRule* rule = node->rule;
      kev_lr_print_non_kernel_item(out, collec, rule, las[head_index]);
      fputc('\n', out);
    }
  }
  return;
}

bool kev_lr_print_itemset(FILE* out, KevLRCollection* collec, KevItemSet* itemset, bool print_closure) {
  for (KevItem* kitem = itemset->items; kitem; kitem = kitem->next) {
    kev_lr_print_kernel_item(out, collec, kitem);
    fputc('\n', out);
  }
  if (!print_closure) return true;
  KevItemSetClosure* closure = kev_lr_closure_create(collec->symbol_no);
  if (!closure) return false;
  if (!kev_lr_closure_make(closure, itemset, collec->firsts, collec->terminal_no)) {
    kev_lr_closure_delete(closure);
    return false;
  }
  KevAddrArray* symbols = closure->symbols;
  KevBitSet** las = closure->lookaheads;
  for (size_t i = 0; i < kev_addrarray_size(symbols); ++i) {
    KevSymbol* head = kev_addrarray_visit(symbols, i);
    size_t head_index = head->tmp_id;
    for (KevRuleNode* node = head->rules; node; node = node->next) {
      KevRule* rule = node->rule;
      kev_lr_print_non_kernel_item(out, collec, rule, las[head_index]);
      fputc('\n', out);
    }
  }
  kev_lr_closure_delete(closure);
  return true;
}

bool kev_lr_print_collection(FILE* out, KevLRCollection* collec, bool print_closure) {
  KevItemSetClosure* closure = kev_lr_closure_create(collec->symbol_no);
  if (!closure) return false;
  for (size_t i = 0; i < collec->itemset_no; ++i) {
    KevItemSet* itemset = collec->itemsets[i];
    if (!kev_lr_closure_make(closure, itemset, collec->firsts, collec->terminal_no)) {
      kev_lr_closure_delete(closure);
      return false;
    }
    fprintf(out, "item set %d:\n", (int)itemset->id);
    kev_lr_print_itemset_with_closure(out, collec, itemset, closure);
    fputc('\n', out);
    kev_lr_closure_make_empty(closure);
  }
  kev_lr_closure_delete(closure);
  return true;
}

bool kev_lr_print_symbols(FILE* out, KevLRCollection* collec) {
  size_t user_symbol_no = kev_lr_collection_get_user_symbol_no(collec);
  KevSymbol** symbol_arrry = (KevSymbol**)malloc(sizeof(KevSymbol*) * user_symbol_no);
  if (!symbol_arrry) return false;
  KevSymbol** symbols = kev_lr_collection_get_symbols(collec);
  memcpy(symbol_arrry, symbols, sizeof (KevSymbol*) * (collec->start->tmp_id));
  memcpy(symbol_arrry, symbols + collec->start->tmp_id + 1, sizeof (KevSymbol*) * (user_symbol_no - collec->start->tmp_id));
  qsort(symbol_arrry, user_symbol_no, sizeof (KevSymbol*), kev_symbol_compare);
  size_t width = 0;
  for (size_t i = 0; i < user_symbol_no; ++i) {
    size_t namelen = symbol_arrry[i] ? strlen(symbol_arrry[i]->name) : (sizeof ("<no name>") / sizeof ("<no name>"[0]) - 1);
    if (namelen > width) width = namelen;
  }
  width = kev_max(width, (sizeof ("SYMBOLS") / sizeof ("SYMBOLS")[0]) - 1);
  width += 1;
  char num_format[sizeof (size_t) * 4];
  char str_format[sizeof (size_t) * 4];
  sprintf(num_format, "%%%llud", width);
  sprintf(str_format, "%%%llus", width);
  fprintf(out, str_format, "SYMBOLS");
  for (size_t i = 0; i < 10; ++i)
    fprintf(out, num_format, (int)i);

  for (size_t i = 0; i < user_symbol_no; ++i) {
    if (i % 10 == 0) {
      fputc('\n', out);
      fprintf(out, num_format, (int)i);
    }
    fprintf(out, str_format, symbol_arrry[i]->name ? symbol_arrry[i]->name : "<no name>");
  }
  fputc('\n', out);
  free(symbol_arrry);
  return true;
}

void kev_lr_print_rule(FILE* out, KevRule* rule) {
  KevSymbol** body = rule->body;
  fprintf(out, "%s -> ", rule->head->name ? rule->head->name : KEV_LR_SYMBOL_UNNAMED);
  fprintf(out, "%s ", KEV_LR_DOT);
  for (size_t i = 0; i < rule->bodylen; ++i) 
    fprintf(out, "%s ", body[i]->name ? body[i]->name : KEV_LR_SYMBOL_UNNAMED);
}

void kev_lr_print_kernel_item(FILE* out, KevLRCollection* collec, KevItem* kitem) {
  KevRule* rule = kitem->rule;
  KevSymbol** body = rule->body;
  size_t len = rule->bodylen;
  fprintf(out, "%s -> ", rule->head->name ? rule->head->name : KEV_LR_SYMBOL_UNNAMED);
  size_t dot = kitem->dot;
  for (size_t i = 0; i < dot; ++i) 
    fprintf(out, "%s ", body[i]->name ? body[i]->name : KEV_LR_SYMBOL_UNNAMED);
  fprintf(out, "%s ", KEV_LR_DOT);
  for (size_t i = dot; i < len; ++i) 
    fprintf(out, "%s ", body[i]->name ? body[i]->name : KEV_LR_SYMBOL_UNNAMED);
  kev_lr_print_terminal_set(out, collec, kitem->lookahead);
}

void kev_lr_print_non_kernel_item(FILE* out, KevLRCollection* collec, KevRule* rule, KevBitSet* lookahead) {
  KevSymbol** body = rule->body;
  size_t len = rule->bodylen;
  fprintf(out, "%s -> ", rule->head->name ? rule->head->name : KEV_LR_SYMBOL_UNNAMED);
  fprintf(out, "%s ", KEV_LR_DOT);
  for (size_t i = 0; i < len; ++i) 
    fprintf(out, "%s ", body[i]->name ? body[i]->name : KEV_LR_SYMBOL_UNNAMED);
  kev_lr_print_terminal_set(out, collec, lookahead);
}

void kev_lr_print_terminal_set(FILE* out, KevLRCollection* collec, KevBitSet* lookahead) {
  if (!lookahead) return;
  if (kev_bitset_empty(lookahead)) {
    fprintf(out, "[]");
    return;
  }
  size_t epsilon = collec->terminal_no;
  size_t symbol_index = kev_bitset_iterate_begin(lookahead);
  size_t next_index = 0;
  char* name = collec->symbols[symbol_index]->name;
  fprintf(out, "[%s", name ? name : KEV_LR_SYMBOL_UNNAMED);
  next_index = kev_bitset_iterate_next(lookahead, symbol_index);
  while (next_index != symbol_index) {
    symbol_index = next_index;
    if (symbol_index != epsilon) {
      char* name = collec->symbols[symbol_index]->name;
      fprintf(out, ", %s", name ? name : KEV_LR_SYMBOL_UNNAMED);
    } else {
      fprintf(out, KEV_LR_SYMBOL_EPSILON_STRING);
    }
    next_index = kev_bitset_iterate_next(lookahead, symbol_index);
  }
  fputc(']', out);
}

void kev_lr_print_goto_table(FILE* out, KevLRTable* table) {
  size_t width = kev_numlen(kev_max(table->symbol_no, table->itemset_no)) + 1;
  width = kev_max(width, 5);
  char num_format[sizeof (size_t) * 4];
  char str_format[sizeof (size_t) * 4];
  sprintf(num_format, "%%%llud", width);
  sprintf(str_format, "%%%llus", width);
  fprintf(out, str_format, "GOTO");
  for (size_t i = 0; i < table->symbol_no; ++i)
    fprintf(out, num_format, (int)i);
  fputc('\n', out);
  for (size_t i = 0; i < table->itemset_no; ++i) {
    fprintf(out, num_format, (int)i);
    for (size_t j = 0; j < table->symbol_no; ++j)
      fprintf(out, num_format, (int)table->entries[i][j].go_to);
    fputc('\n', out);
  }
}

void kev_lr_print_action_table(FILE* out, KevLRTable* table) {
  size_t width = kev_numlen(kev_max(table->symbol_no, table->itemset_no)) + 1;
  width = kev_max(width, 7);
  char num_format[sizeof (size_t) * 4];
  char str_format[sizeof (size_t) * 4];
  sprintf(num_format, "%%%llud", width);
  sprintf(str_format, "%%%llus", width);
  fprintf(out, str_format, "ACTION");
  for (size_t i = 0; i < table->symbol_no; ++i)
    fprintf(out, num_format, (int)i);
  fputc('\n', out);

  for (size_t i = 0; i < table->itemset_no; ++i) {
    fprintf(out, num_format, (int)i);
    for (size_t j = 0; j < table->symbol_no; ++j) {
      char* action = NULL;
      switch (table->entries[i][j].action) {
        case KEV_LR_ACTION_ACC: action = "ACC"; break;
        case KEV_LR_ACTION_RED: action = "RED"; break;
        case KEV_LR_ACTION_SHI: action = "SHI"; break;
        case KEV_LR_ACTION_ERR: action = "ERR"; break;
        case KEV_LR_ACTION_CON: action = "CON"; break;
        default: action = "NUL"; break;
      }
      fprintf(out, str_format, action);
    }
    fputc('\n', out);
    fprintf(out, str_format, " ");
    for (size_t j = 0; j < table->symbol_no; ++j) {
      KevLRTableEntry* entry = &table->entries[i][j];
      if (entry->action == KEV_LR_ACTION_SHI) {
        fprintf(out, num_format, entry->info.itemset_id);
      } else if (entry->action == KEV_LR_ACTION_RED) {
        fprintf(out, num_format, entry->info.rule->id);
      } else if (entry->action == KEV_LR_ACTION_CON) {
        bool has_sr = kev_lr_conflict_SR(entry->info.conflict);
        bool has_rr = kev_lr_conflict_RR(entry->info.conflict);
        if (has_sr && has_rr) {
          fprintf(out, str_format, "RSR");
        } else if (has_sr) {
          fprintf(out, str_format, "SR");
        } else {
          fprintf(out, str_format, "RR");
        }
      }
      else
        fprintf(out, str_format, " ");
    }
    fputc('\n', out);
  }
}
