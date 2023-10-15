#include "kevlr/include/lr_utils.h"
#include "utils/include/set/hashset.h"

#include <stdlib.h>

#define KLR_AUGMENTED_GRAMMAR_START_SYMBOL_NAME   "G"


static void klr_util_compute_first(KBitSet** firsts, KlrSymbol* symbol, size_t epsilon);

static inline bool klr_util_symbol_is_in_array(KlrSymbol* symbol, KArray* array);

bool klr_util_generate_transition(KlrItemSet* itemset, KlrItemSetClosure* closure, KlrTransMap* transitions) {
  klr_transmap_make_empty(transitions);
  KArray* symbols = closure->symbols;
  KBitSet** las = closure->lookaheads;
  /* for kernel item */
  KlrItem* kitem = itemset->items;
  for (; kitem; kitem = kitem->next) {
    KlrRule* rule = kitem->rule;
    if (rule->bodylen == kitem->dot) continue;
    KlrSymbol* symbol = rule->body[kitem->dot];
    KlrItem* item = klr_item_create(rule, kitem->dot + 1);
    if (!item) return false;
    if (!(item->lookahead = kbitset_create_copy(kitem->lookahead))) {
      klr_item_delete(item);
      return false;
    }
    KlrTransMapNode* node = klr_transmap_search(transitions, symbol);
    if (node) {
      klr_itemset_add_item(node->value, item);
    } else {
      KlrItemSet* iset = klr_itemset_create();
      if (!iset) {
        klr_item_delete(item);
        return false;
      }
      klr_itemset_add_item(iset, item);
      if (!klr_itemset_goto(itemset, symbol, iset) ||
          !klr_transmap_insert(transitions, symbol, iset)) {
        klr_itemset_delete(iset);
        return false;
      }
    }
  }

  /* for non-kernel item */
  size_t closure_size = karray_size(symbols);
  for (size_t i = 0; i < closure_size; ++i) {
    KlrSymbol* head = (KlrSymbol*)karray_access(symbols, i);
    KlrRuleNode* rulenode = head->rules;
    for (; rulenode; rulenode = rulenode->next) {
      KlrRule* rule = rulenode->rule;
      if (rule->bodylen == 0) continue;
      KlrSymbol* symbol = rule->body[0];
      KlrItem* item = klr_item_create(rule, 1);
      if (!item) return false;
      if (!(item->lookahead = kbitset_create_copy(las[head->index]))) {
        klr_item_delete(item);
        return false;
      }
      KlrTransMapNode* node = klr_transmap_search(transitions, symbol);
      if (node) {
        klr_itemset_add_item(node->value, item);
      } else {
        KlrItemSet* iset = klr_itemset_create();
        if (!iset) {
          klr_item_delete(item);
          return false;
        }
        klr_itemset_add_item(iset, item);
        if (!klr_itemset_goto(itemset, symbol, iset) ||
            !klr_transmap_insert(transitions, symbol, iset)) {
          klr_itemset_delete(iset);
          return false;
        }
      }
    }
  }
  return true;
}

static void klr_util_compute_first(KBitSet** firsts, KlrSymbol* symbol, size_t epsilon) {
  KBitSet* first = firsts[symbol->index];
    for (KlrRuleNode* node = symbol->rules; node; node = node->next) {
      KlrRule* rule = node->rule;
      size_t bodylen = rule->bodylen;
      KlrSymbol** body = rule->body;
      size_t i = 0;
      bool first_has_epsilon = kbitset_has_element(first, epsilon);
      for (; i < bodylen; ++i) {
        if (body[i]->kind == KLR_TERMINAL) {
          kbitset_set(first, body[i]->index);
          break;
        }
        KBitSet* curr = firsts[body[i]->index];
        /* all first sets has same size, so union will never fail */
        kbitset_union(first, curr);
        if (!kbitset_has_element(curr, epsilon)) {
          if (!first_has_epsilon)
            kbitset_clear(first, epsilon);
          break;
        }
      }
      if (i == bodylen)
        kbitset_set(first, epsilon);
    }
}

KBitSet** klr_util_compute_firsts(KlrSymbol** symbols, size_t symbol_no, size_t terminal_no) {
  KBitSet** firsts = (KBitSet**)malloc(sizeof (KBitSet*) * symbol_no);
  if (!firsts) return NULL;
  KBitSet backup;
  if (!kbitset_init(&backup, terminal_no + 1)) {
    free (firsts);
    return NULL;
  }
  /* initialize */
  for (size_t i = 0; i < terminal_no; ++i)
    firsts[i] = NULL;
  for (size_t i = terminal_no; i < symbol_no; ++i) {
    if (!(firsts[i] = kbitset_create(terminal_no + 1))) {
      for (size_t j = terminal_no; j < i; ++j)
        kbitset_delete(firsts[j]);
      free(firsts);
      kbitset_destroy(&backup);
      return NULL;
    }
  }
  /* compute firsts */
  bool loop = true;
  while (loop) {
    loop = false;
    for (size_t i = terminal_no; i < symbol_no; ++i) {
      if (!loop) kbitset_assign(&backup, firsts[i]);
      klr_util_compute_first(firsts, symbols[i], terminal_no);
      if (!loop && !kbitset_equal(&backup, firsts[i]))
        loop = true;
    }
  }
  kbitset_destroy(&backup);
  return firsts;
}

KlrSymbol* klr_util_augment(KlrSymbol* start) {
  KlrSymbol* new_start = klr_symbol_create(KLR_NONTERMINAL, KLR_AUGMENTED_GRAMMAR_START_SYMBOL_NAME);
  if (!new_start) return NULL;
  KlrRule* start_rule = klr_rule_create(new_start, &start, 1);
  if (!start_rule) {
    klr_symbol_delete(new_start);
    return NULL;
  }
  return new_start;
}

KBitSet* klr_util_symbols_to_bitset(KlrSymbol** symbols, size_t length) {
  KBitSet* set = kbitset_create(1);
  if (!set) return NULL;
  for (size_t i = 0; i < length; ++i) {
    if (!kbitset_set(set, symbols[i]->index)) {
      kbitset_delete(set);
      return NULL;
    }
  }
  return set;
}

KlrItemSet* klr_util_get_start_itemset(KlrSymbol* start, KlrSymbol** lookahead, size_t length) {
  KBitSet* la = klr_util_symbols_to_bitset(lookahead, length);
  KlrItemSet* iset = klr_itemset_create();
  if (!iset || !la) {
    kbitset_delete(la);
    klr_itemset_delete(iset);
    return NULL;
  }
  for (KlrRuleNode* node = start->rules; node; node = node->next) {
    KlrItem* item = klr_item_create(node->rule, 0);
    if (!item || !(item->lookahead = kbitset_create_copy(la))) {
      klr_itemset_delete(iset);
      kbitset_delete(la);
      return NULL;
    }
    klr_itemset_add_item(iset, item);
  }
  kbitset_delete(la);
  return iset;
}

void klr_util_destroy_terminal_set_array(KBitSet** firsts, size_t size) {
  for (size_t i = 0; i < size; ++i)
    kbitset_delete(firsts[i]);
  free(firsts);
}

static inline bool klr_util_symbol_is_in_array(KlrSymbol* symbol, KArray* array) {
  return symbol->index < karray_size(array) && karray_access(array, symbol->index) == symbol;
}

KlrSymbol** klr_util_get_symbol_array(KlrSymbol* start, KlrSymbol** ends, size_t ends_no, size_t* p_size) {
  KArray array;
  if (!karray_init(&array))
    return NULL;
  if (!karray_push_back(&array, start)) {
    karray_destroy(&array);
    return NULL;
  }
  start->index = 0;

  for (size_t curr = 0; curr != karray_size(&array); ++curr) {
    KlrSymbol* symbol = (KlrSymbol*)karray_access(&array, curr);
    for (KlrRuleNode* node = symbol->rules; node; node = node->next) {
      KlrRule* rule = node->rule;
      size_t bodylen = klr_rule_get_bodylen(rule);
      KlrSymbol** body = klr_rule_get_body(rule);
      for (size_t i = 0; i < bodylen; ++i) {
        if (klr_util_symbol_is_in_array(body[i], &array))
          continue;
        body[i]->index = karray_size(&array);
        if (!karray_push_back(&array, body[i])) {
          karray_destroy(&array);
          return NULL;
        }
      }
    }
  }

  for (size_t i = 0; i < ends_no; ++i) {
    if (klr_util_symbol_is_in_array(ends[i], &array))
      continue;
    ends[i]->index = karray_size(&array);
    if (!karray_push_back(&array, ends[i])) {
      karray_destroy(&array);
      return NULL;
    }
  }

  *p_size = karray_size(&array);
  KlrSymbol** symbol_array = (KlrSymbol**)karray_steal(&array);
  karray_destroy(&array);
  return symbol_array;
}

KlrSymbol** klr_util_get_symbol_array_with_index_unchanged(KlrSymbol* start, KlrSymbol** ends, size_t ends_no, size_t* p_size) {
  KArray array;
  if (!karray_init(&array))
    return NULL;

  KevHashSet set;
  if (!kev_hashset_init(&set, 64)) {
    kev_hashset_destroy(&set);
    return NULL;
  }
  if (!kev_hashset_insert(&set, start) ||
      !karray_push_back(&array, start)) {
    kev_hashset_destroy(&set);
    karray_destroy(&array);
    return NULL;
  }
  size_t curr = 0;

  while (curr != karray_size(&array)) {
    KlrSymbol* symbol = (KlrSymbol*)karray_access(&array, curr++);
    KlrRuleNode* rule = symbol->rules;
    for (; rule; rule = rule->next) {
      KlrSymbol** rule_body = rule->rule->body;
      size_t len = rule->rule->bodylen;
      for (size_t i = 0; i < len; ++i) {
        if (kev_hashset_has(&set, rule_body[i]))
          continue;
        if (!kev_hashset_insert(&set, rule_body[i]) ||
            !karray_push_back(&array, rule_body[i])) {
          kev_hashset_destroy(&set);
          karray_destroy(&array);
          return NULL;
        }
      }
    }
  }

  for (size_t i = 0; i < ends_no; ++i) {
    if (kev_hashset_has(&set, ends[i]))
      continue;
    if (!kev_hashset_insert(&set, ends[i]) ||
        !karray_push_back(&array, ends[i])) {
      kev_hashset_destroy(&set);
      karray_destroy(&array);
      return NULL;
    }
  }

  *p_size = karray_size(&array);
  KlrSymbol** symbol_array = (KlrSymbol**)karray_steal(&array);
  karray_destroy(&array);
  kev_hashset_destroy(&set);
  return symbol_array;
}

size_t klr_util_symbol_array_partition(KlrSymbol** array, size_t size) {
  KlrSymbol** left = array;
  KlrSymbol** right = array + size - 1;
  while (true) {
    while (left < right && (*left)->kind == KLR_TERMINAL)
      ++left;
    while (left < right && (*right)->kind == KLR_NONTERMINAL)
      --right;
    if (left >= right) break;
    KlrSymbol* tmp = *left;
    *left = *right;
    *right = tmp;
  }
  return (*left)->kind == KLR_TERMINAL ? left - array + 1 : left - array;
}

KBitSet** klr_util_compute_follows(KlrSymbol** symbols, KBitSet** firsts, size_t symbol_no, size_t terminal_no, KlrSymbol* start, KlrSymbol** ends, size_t ends_no) {
  KBitSet curr_follow;
  if (!kbitset_init(&curr_follow, terminal_no + 1))
    return NULL;
  KBitSet** follows = (KBitSet**)malloc(sizeof (KBitSet*) * symbol_no);
  if (!follows) {
    kbitset_destroy(&curr_follow);
    return NULL;
  }
  /* initialize */
  for (size_t i = 0; i < terminal_no; ++i)
    follows[i] = NULL;
  for (size_t i = terminal_no; i < symbol_no; ++i) {
    if (!(follows[i] = kbitset_create(terminal_no + 1))) {
      for (size_t j = terminal_no; j < i; ++j)
        kbitset_delete(follows[j]);
      free(follows);
      kbitset_destroy(&curr_follow);
      return NULL;
    }
  }
  for (size_t i = 0; i < ends_no; ++i)
    kbitset_set(follows[start->index], ends[i]->index);

  bool loop = true;
  size_t epsilon = terminal_no;
  while (loop) {
    loop = false;
    for (size_t i = 0; i < symbol_no; ++i) {
      KlrSymbol* head = symbols[i];
      KBitSet* head_follow = follows[head->index];
      for (KlrRuleNode* rulenode = head->rules; rulenode; rulenode = rulenode->next) {
        KlrSymbol** body = rulenode->rule->body;
        size_t len = rulenode->rule->bodylen;
        if (len == 0) continue;
        size_t i = len;
        kbitset_assign(&curr_follow, head_follow);
        do {
          --i;
          KlrSymbol* symbol = body[i];
          if (symbol->kind == KLR_TERMINAL) {
            kbitset_make_empty(&curr_follow);
            kbitset_set(&curr_follow, symbol->index);
          } else {
            if (kbitset_changed_after_shrinking_union(follows[symbol->index], &curr_follow))
              loop = true;
            if (kbitset_has_element(firsts[symbol->index], epsilon)) {
              kbitset_union(&curr_follow, firsts[symbol->index]);
              kbitset_clear(&curr_follow, epsilon);
            } else {
              kbitset_assign(&curr_follow, firsts[symbol->index]);
            }
          }
        } while (i != 0);
      }
    }
  }

  kbitset_destroy(&curr_follow);
  return follows;
}

size_t klr_util_user_symbol_max_id(KlrCollection* collec) {
  size_t max_id = 0;
  KlrSymbol** symbols = klr_collection_get_symbols(collec);
  size_t symbol_no = collec->symbol_no;
  for (size_t i = 0; i < symbol_no; ++i) {
    /* collec->start is not defined by user */
    if (symbols[i] != collec->start && max_id < symbols[i]->id)
      max_id = symbols[i]->id;
  }
  return max_id;
}

size_t klr_util_user_terminal_max_id(KlrCollection* collec) {
  size_t max_id = 0;
  KlrSymbol** symbols = klr_collection_get_symbols(collec);
  size_t terminal_no = collec->terminal_no;
  for (size_t i = 0; i < terminal_no; ++i) {
    /* collec->start is non-terminal */
    if (max_id < symbols[i]->id)
      max_id = symbols[i]->id;
  }
  return max_id;
}

size_t klr_util_user_nonterminal_max_id(KlrCollection* collec) {
  size_t max_id = 0;
  KlrSymbol** symbols = klr_collection_get_symbols(collec);
  size_t symbol_no = collec->symbol_no;
  for (size_t i = collec->terminal_no; i < symbol_no; ++i) {
    /* collec->start is not defined by user */
    if (symbols[i] != collec->start && max_id < symbols[i]->id)
      max_id = symbols[i]->id;
  }
  return max_id;
}
