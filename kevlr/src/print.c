#include "kevlr/include/print.h"
#include "kevlr/include/lr_utils.h"

#include <stdlib.h>
#include <string.h>

#define KLR_SYMBOL_EPSILON_STRING  "ε"
#define KLR_DOT                    "·"
#define KLR_SYMBOL_UNNAMED         "[UNNAMED]"

static inline size_t kev_numlen(size_t num);
static inline size_t kev_max(size_t num1, size_t num2);
static int kev_symbol_compare(const void* sym1, const void* sym2);


static inline size_t kev_numlen(size_t num) {
  char numstr[sizeof (size_t) * 4];
  return sprintf(numstr, "%llu", (unsigned long long)num);
}

static inline size_t kev_max(size_t num1, size_t num2) {
  return num1 > num2 ? num1 : num2;
}

static int kev_symbol_compare(const void* sym1, const void* sym2) {
  return (*(KlrSymbol**)sym1)->id - (*(KlrSymbol**)sym2)->id;
}

void klr_print_itemset_with_closure(FILE* out, KlrCollection* collec, KlrItemSet* itemset, KlrItemSetClosure* closure) {
  for (KlrItem* kitem = itemset->items; kitem; kitem = kitem->next) {
    klr_print_kernel_item(out, collec, kitem);
    fputc('\n', out);
  }
  if (!closure) return;
  KArray* symbols = closure->symbols;
  KBitSet** las = closure->lookaheads;
  for (size_t i = 0; i < karray_size(symbols); ++i) {
    KlrSymbol* head = (KlrSymbol*)karray_access(symbols, i);
    size_t head_index = head->index;
    for (KlrRuleNode* node = head->rules; node; node = node->next) {
      KlrRule* rule = node->rule;
      klr_print_non_kernel_item(out, collec, rule, las[head_index]);
      fputc('\n', out);
    }
  }
  return;
}

bool klr_print_itemset(FILE* out, KlrCollection* collec, KlrItemSet* itemset, bool print_closure) {
  for (KlrItem* kitem = itemset->items; kitem; kitem = kitem->next) {
    klr_print_kernel_item(out, collec, kitem);
    fputc('\n', out);
  }
  if (!print_closure) return true;
  KlrItemSetClosure* closure = klr_closure_create(collec->nsymbol);
  if (!closure) return false;
  if (!klr_closure_make(closure, itemset, collec->firsts, collec->nterminal)) {
    klr_closure_delete(closure);
    return false;
  }
  KArray* symbols = closure->symbols;
  KBitSet** las = closure->lookaheads;
  for (size_t i = 0; i < karray_size(symbols); ++i) {
    KlrSymbol* head = (KlrSymbol*)karray_access(symbols, i);
    size_t head_index = head->index;
    for (KlrRuleNode* node = head->rules; node; node = node->next) {
      KlrRule* rule = node->rule;
      klr_print_non_kernel_item(out, collec, rule, las[head_index]);
      fputc('\n', out);
    }
  }
  klr_closure_delete(closure);
  return true;
}

bool klr_print_collection(FILE* out, KlrCollection* collec, bool print_closure) {
  if (!print_closure) {
    for (size_t i = 0; i < collec->nitemset; ++i) {
      KlrItemSet* itemset = collec->itemsets[i];
      fprintf(out, "item set %d:\n", (int)itemset->id);
      klr_print_itemset_with_closure(out, collec, itemset, NULL);
      fputc('\n', out);
    }
  }

  KlrItemSetClosure* closure = klr_closure_create(collec->nsymbol);
  if (!closure) return false;
  for (size_t i = 0; i < collec->nitemset; ++i) {
    KlrItemSet* itemset = collec->itemsets[i];
    if (!klr_closure_make(closure, itemset, collec->firsts, collec->nterminal)) {
      klr_closure_delete(closure);
      return false;
    }
    fprintf(out, "item set %d:\n", (int)itemset->id);
    klr_print_itemset_with_closure(out, collec, itemset, closure);
    fputc('\n', out);
    klr_closure_make_empty(closure);
  }
  klr_closure_delete(closure);
  return true;
}

bool klr_print_symbols(FILE* out, KlrCollection* collec) {
  size_t nusersymbol = klr_collection_nusersymbol(collec);
  KlrSymbol** symbol_array = (KlrSymbol**)malloc(sizeof(KlrSymbol*) * nusersymbol);
  if (!symbol_array) return false;
  KlrSymbol** symbols = klr_collection_get_symbols(collec);
  memcpy(symbol_array, symbols, sizeof (KlrSymbol*) * (collec->start->index));
  memcpy(symbol_array, symbols + collec->start->index + 1, sizeof (KlrSymbol*) * (nusersymbol - collec->start->index));
  qsort(symbol_array, nusersymbol, sizeof (KlrSymbol*), kev_symbol_compare);
  size_t width = 0;
  for (size_t i = 0; i < nusersymbol; ++i) {
    size_t namelen = symbol_array[i] ? strlen(symbol_array[i]->name) : (sizeof ("<no name>") / sizeof ("<no name>"[0]) - 1);
    if (namelen > width) width = namelen;
  }
  width = kev_max(width, (sizeof ("SYMBOLS") / sizeof ("SYMBOLS")[0]) - 1);
  width += 1;
  char num_format[sizeof (size_t) * 4];
  char str_format[sizeof (size_t) * 4];
  sprintf(num_format, "%%%llud", (unsigned long long)width);
  sprintf(str_format, "%%%llus", (unsigned long long)width);
  fprintf(out, str_format, "SYMBOLS");
  for (size_t i = 0; i < 10; ++i)
    fprintf(out, num_format, (int)i);

  size_t max_id = klr_util_user_symbol_max_id(collec);
  for (size_t id = 0, i = 0; id <= max_id; ++id) {
    if (id % 10 == 0) {
      fputc('\n', out);
      fprintf(out, num_format, (int)id);
    }
    if (symbol_array[i]->id == id) {
      fprintf(out, str_format, symbol_array[i]->name ? symbol_array[i]->name : "<no name>");
      ++i;
    } else {
      fprintf(out, str_format, "");
    }
  }
  fputc('\n', out);
  free(symbol_array);
  return true;
}

void klr_print_rule(FILE* out, KlrRule* rule) {
  KlrSymbol** body = rule->body;
  fprintf(out, "%s -> ", rule->head->name ? rule->head->name : KLR_SYMBOL_UNNAMED);
  fprintf(out, "%s ", KLR_DOT);
  for (size_t i = 0; i < rule->bodylen; ++i) 
    fprintf(out, "%s ", body[i]->name ? body[i]->name : KLR_SYMBOL_UNNAMED);
}

void klr_print_kernel_item(FILE* out, KlrCollection* collec, KlrItem* kitem) {
  KlrRule* rule = kitem->rule;
  KlrSymbol** body = rule->body;
  size_t len = rule->bodylen;
  fprintf(out, "%s -> ", rule->head->name ? rule->head->name : KLR_SYMBOL_UNNAMED);
  size_t dot = kitem->dot;
  for (size_t i = 0; i < dot; ++i) 
    fprintf(out, "%s ", body[i]->name ? body[i]->name : KLR_SYMBOL_UNNAMED);
  fprintf(out, "%s ", KLR_DOT);
  for (size_t i = dot; i < len; ++i) 
    fprintf(out, "%s ", body[i]->name ? body[i]->name : KLR_SYMBOL_UNNAMED);
  fputc(' ', out);
  klr_print_terminal_set(out, collec, kitem->lookahead);
}

void klr_print_non_kernel_item(FILE* out, KlrCollection* collec, KlrRule* rule, KBitSet* lookahead) {
  KlrSymbol** body = rule->body;
  size_t len = rule->bodylen;
  fprintf(out, "%s -> ", rule->head->name ? rule->head->name : KLR_SYMBOL_UNNAMED);
  fprintf(out, "%s ", KLR_DOT);
  for (size_t i = 0; i < len; ++i) 
    fprintf(out, "%s ", body[i]->name ? body[i]->name : KLR_SYMBOL_UNNAMED);
  fputc(' ', out);
  klr_print_terminal_set(out, collec, lookahead);
}

void klr_print_terminal_set(FILE* out, KlrCollection* collec, KBitSet* lookahead) {
  if (!lookahead) return;
  if (kbitset_empty(lookahead)) {
    fprintf(out, "[]");
    return;
  }
  size_t epsilon = collec->nterminal;
  size_t symbol_index = kbitset_iter_begin(lookahead);
  size_t next_index = 0;
  char* name = collec->symbols[symbol_index]->name;
  fprintf(out, "[%s", name ? name : KLR_SYMBOL_UNNAMED);
  next_index = kbitset_iter_next(lookahead, symbol_index);
  while (next_index != symbol_index) {
    symbol_index = next_index;
    if (symbol_index != epsilon) {
      char* name = collec->symbols[symbol_index]->name;
      fprintf(out, ", %s", name ? name : KLR_SYMBOL_UNNAMED);
    } else {
      fprintf(out, ", %s", KLR_SYMBOL_EPSILON_STRING);
    }
    next_index = kbitset_iter_next(lookahead, symbol_index);
  }
  fputc(']', out);
}

void klr_print_trans_table(FILE* out, KlrTable* table) {
  size_t width = kev_numlen(kev_max(table->ntblsymbol, table->nstate)) + 1;
  width = kev_max(width, 5);
  char num_format[sizeof (size_t) * 4];
  char str_format[sizeof (size_t) * 4];
  sprintf(num_format, "%%%llud", (unsigned long long)width);
  sprintf(str_format, "%%%llus", (unsigned long long)width);
  fprintf(out, str_format, "GOTO");
  for (size_t i = 0; i < table->ntblsymbol; ++i)
    fprintf(out, num_format, (int)i);
  fputc('\n', out);
  for (size_t i = 0; i < table->nstate; ++i) {
    fprintf(out, num_format, (int)i);
    for (size_t j = 0; j < table->ntblsymbol; ++j)
      fprintf(out, num_format, (int)table->entries[i][j].trans);
    fputc('\n', out);
  }
}

void klr_print_action_table(FILE* out, KlrTable* table) {
  size_t width = kev_numlen(kev_max(table->ntblsymbol, table->nstate)) + 1;
  width = kev_max(width, 7);
  char num_format[sizeof (size_t) * 4];
  char str_format[sizeof (size_t) * 4];
  sprintf(num_format, "%%%llud", (unsigned long long)width);
  sprintf(str_format, "%%%llus", (unsigned long long)width);
  fprintf(out, str_format, "ACTION");
  for (size_t i = 0; i < table->ntblsymbol; ++i)
    fprintf(out, num_format, (int)i);
  fputc('\n', out);

  for (size_t i = 0; i < table->nstate; ++i) {
    fprintf(out, num_format, (int)i);
    for (size_t j = 0; j < table->ntblsymbol; ++j) {
      const char* action = NULL;
      switch (table->entries[i][j].action) {
        case KLR_ACTION_ACC: action = "ACC"; break;
        case KLR_ACTION_RED: action = "RED"; break;
        case KLR_ACTION_SHI: action = "SHI"; break;
        case KLR_ACTION_ERR: action = "ERR"; break;
        case KLR_ACTION_CON: action = "CON"; break;
        default: action = "NUL"; break;
      }
      fprintf(out, str_format, action);
    }
    fputc('\n', out);
    fprintf(out, str_format, " ");
    for (size_t j = 0; j < table->ntblsymbol; ++j) {
      KlrTableEntry* entry = &table->entries[i][j];
      if (entry->action == KLR_ACTION_SHI) {
        fprintf(out, num_format, entry->info.itemset_id);
      } else if (entry->action == KLR_ACTION_RED) {
        fprintf(out, num_format, entry->info.rule->id);
      } else if (entry->action == KLR_ACTION_CON) {
        bool has_sr = klr_conflict_SR(entry->info.conflict);
        bool has_rr = klr_conflict_RR(entry->info.conflict);
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
