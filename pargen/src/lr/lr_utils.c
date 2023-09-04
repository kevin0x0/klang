#include "pargen/include/lr/lr_utils.h"
#include "utils/include/set/hashset.h"

#include <stdlib.h>

#define KEV_LR_AUGMENTED_GRAMMAR_START_SYMBOL_NAME   "G"


static void kev_lr_util_compute_first(KevBitSet** firsts, KevSymbol* symbol, size_t epsilon);

bool kev_lr_util_generate_gotos(KevItemSet* itemset, KevItemSetClosure* closure, KevGotoMap* goto_container) {
  kev_gotomap_make_empty(goto_container);
  KevAddrArray* symbols = closure->symbols;
  KevBitSet** las = closure->lookaheads;
  /* for kernel item */
  KevItem* kitem = itemset->items;
  for (; kitem; kitem = kitem->next) {
    KevRule* rule = kitem->rule;
    if (rule->bodylen == kitem->dot) continue;
    KevSymbol* symbol = rule->body[kitem->dot];
    KevItem* item = kev_lr_item_create(rule, kitem->dot + 1);
    if (!item) return false;
    if (!(item->lookahead = kev_bitset_create_copy(kitem->lookahead))) {
      kev_lr_item_delete(item);
      return false;
    }
    KevGotoMapNode* node = kev_gotomap_search(goto_container, symbol);
    if (node) {
      kev_lr_itemset_add_item(node->value, item);
    } else {
      KevItemSet* iset = kev_lr_itemset_create();
      if (!iset) {
        kev_lr_item_delete(item);
        return false;
      }
      kev_lr_itemset_add_item(iset, item);
      if (!kev_lr_itemset_goto(itemset, symbol, iset) ||
          !kev_gotomap_insert(goto_container, symbol, iset)) {
        kev_lr_itemset_delete(iset);
        return false;
      }
    }
  }

  /* for non-kernel item */
  size_t closure_size = kev_addrarray_size(symbols);
  for (size_t i = 0; i < closure_size; ++i) {
    KevSymbol* head = (KevSymbol*)kev_addrarray_visit(symbols, i);
    KevRuleNode* rulenode = head->rules;
    for (; rulenode; rulenode = rulenode->next) {
      KevRule* rule = rulenode->rule;
      if (rule->bodylen == 0) continue;
      KevSymbol* symbol = rule->body[0];
      KevItem* item = kev_lr_item_create(rule, 1);
      if (!item) return false;
      if (!(item->lookahead = kev_bitset_create_copy(las[head->tmp_id]))) {
        kev_lr_item_delete(item);
        return false;
      }
      KevGotoMapNode* node = kev_gotomap_search(goto_container, symbol);
      if (node) {
        kev_lr_itemset_add_item(node->value, item);
      } else {
        KevItemSet* iset = kev_lr_itemset_create();
        if (!iset) {
          kev_lr_item_delete(item);
          return false;
        }
        kev_lr_itemset_add_item(iset, item);
        if (!kev_lr_itemset_goto(itemset, symbol, iset) ||
            !kev_gotomap_insert(goto_container, symbol, iset)) {
          kev_lr_itemset_delete(iset);
          return false;
        }
      }
    }
  }
  return true;
}

static void kev_lr_util_compute_first(KevBitSet** firsts, KevSymbol* symbol, size_t epsilon) {
  KevBitSet* first = firsts[symbol->tmp_id];
    for (KevRuleNode* node = symbol->rules; node; node = node->next) {
      KevRule* rule = node->rule;
      size_t bodylen = rule->bodylen;
      KevSymbol** body = rule->body;
      size_t i = 0;
      bool first_has_epsilon = kev_bitset_has_element(first, epsilon);
      for (; i < bodylen; ++i) {
        if (body[i]->kind == KEV_LR_SYMBOL_TERMINAL) {
          kev_bitset_set(first, body[i]->tmp_id);
          break;
        }
        KevBitSet* curr = firsts[body[i]->tmp_id];
        /* all first sets has same size, so union will never fail */
        kev_bitset_union(first, curr);
        if (!kev_bitset_has_element(curr, epsilon)) {
          if (!first_has_epsilon)
            kev_bitset_clear(first, epsilon);
          break;
        }
      }
      if (i == bodylen)
        kev_bitset_set(first, epsilon);
    }
}

KevBitSet** kev_lr_util_compute_firsts(KevSymbol** symbols, size_t symbol_no, size_t terminal_no) {
  KevBitSet** firsts = (KevBitSet**)malloc(sizeof (KevBitSet*) * symbol_no);
  if (!firsts) return NULL;
  KevBitSet backup;
  if (!kev_bitset_init(&backup, terminal_no + 1)) {
    free (firsts);
    return NULL;
  }
  /* initialize */
  for (size_t i = 0; i < terminal_no; ++i)
    firsts[i] = NULL;
  for (size_t i = terminal_no; i < symbol_no; ++i) {
    if (!(firsts[i] = kev_bitset_create(terminal_no + 1))) {
      for (size_t j = terminal_no; j < i; ++j)
        kev_bitset_delete(firsts[j]);
      free(firsts);
      kev_bitset_destroy(&backup);
      return NULL;
    }
  }
  /* compute firsts */
  bool loop = true;
  while (loop) {
    loop = false;
    for (size_t i = terminal_no; i < symbol_no; ++i) {
      if (!loop) kev_bitset_assign(&backup, firsts[i]);
      kev_lr_util_compute_first(firsts, symbols[i], terminal_no);
      if (!loop && !kev_bitset_equal(&backup, firsts[i]))
        loop = true;
    }
  }
  kev_bitset_destroy(&backup);
  return firsts;
}

KevSymbol* kev_lr_util_augment(KevSymbol* start) {
  KevSymbol* new_start = kev_lr_symbol_create(KEV_LR_SYMBOL_NONTERMINAL, KEV_LR_AUGMENTED_GRAMMAR_START_SYMBOL_NAME);
  if (!new_start) return NULL;
  KevRule* start_rule = kev_lr_rule_create(new_start, &start, 1);
  if (!start_rule) {
    kev_lr_symbol_delete(new_start);
    return NULL;
  }
  return new_start;
}

KevBitSet* kev_lr_util_symbols_to_bitset(KevSymbol** symbols, size_t length) {
  KevBitSet* set = kev_bitset_create(1);
  if (!set) return NULL;
  for (size_t i = 0; i < length; ++i) {
    if (!kev_bitset_set(set, symbols[i]->tmp_id)) {
      kev_bitset_delete(set);
      return NULL;
    }
  }
  return set;
}

KevItemSet* kev_lr_util_get_start_itemset(KevSymbol* start, KevSymbol** lookahead, size_t length) {
  KevBitSet* la = kev_lr_util_symbols_to_bitset(lookahead, length);
  KevItemSet* iset = kev_lr_itemset_create();
  if (!iset || !la) {
    kev_bitset_delete(la);
    kev_lr_itemset_delete(iset);
    return NULL;
  }
  for (KevRuleNode* node = start->rules; node; node = node->next) {
    KevItem* item = kev_lr_item_create(node->rule, 0);
    if (!item || !(item->lookahead = kev_bitset_create_copy(la))) {
      kev_lr_itemset_delete(iset);
      kev_bitset_delete(la);
      return NULL;
    }
    kev_lr_itemset_add_item(iset, item);
  }
  kev_bitset_delete(la);
  return iset;
}

size_t kev_lr_util_label_symbols(KevSymbol** symbols, size_t symbol_no) {
  size_t number = 0;
  size_t i = 0;
  while (symbols[i]->kind == KEV_LR_SYMBOL_TERMINAL) {
    symbols[i]->tmp_id = i;
    ++i;
  }
  number = i;
  for (; i < symbol_no; ++i)
    symbols[i]->tmp_id = i;
  return number;
}

void kev_lr_util_destroy_terminal_set_array(KevBitSet** firsts, size_t size) {
  for (size_t i = 0; i < size; ++i)
    kev_bitset_delete(firsts[i]);
  free(firsts);
}

KevSymbol** kev_lr_util_get_symbol_array(KevSymbol* start, KevSymbol** ends, size_t ends_no, size_t* p_size) {
  KevAddrArray array;
  if (!kev_addrarray_init(&array))
    return NULL;

  KevHashSet set;
  if (!kev_hashset_init(&set, 64)) {
    kev_hashset_destroy(&set);
    return NULL;
  }
  if (!kev_hashset_insert(&set, start) ||
      !kev_addrarray_push_back(&array, start)) {
    kev_hashset_destroy(&set);
    kev_addrarray_destroy(&array);
    return NULL;
  }
  size_t curr = 0;

  while (curr != kev_addrarray_size(&array)) {
    KevSymbol* symbol = kev_addrarray_visit(&array, curr++);
    KevRuleNode* rule = symbol->rules;
    for (; rule; rule = rule->next) {
      KevSymbol** rule_body = rule->rule->body;
      size_t len = rule->rule->bodylen;
      for (size_t i = 0; i < len; ++i) {
        if (kev_hashset_has(&set, rule_body[i]))
          continue;
        if (!kev_hashset_insert(&set, rule_body[i]) ||
            !kev_addrarray_push_back(&array, rule_body[i])) {
          kev_hashset_destroy(&set);
          kev_addrarray_destroy(&array);
          return NULL;
        }
      }
    }
  }

  for (size_t i = 0; i < ends_no; ++i) {
    if (kev_hashset_has(&set, ends[i]))
      continue;
    if (ends[i]->kind == KEV_LR_SYMBOL_NONTERMINAL) {
      kev_hashset_destroy(&set);
      kev_addrarray_destroy(&array);
      return NULL;
    }
    if (!kev_hashset_insert(&set, ends[i]) ||
        !kev_addrarray_push_back(&array, ends[i])) {
      kev_hashset_destroy(&set);
      kev_addrarray_destroy(&array);
      return NULL;
    }
  }

  kev_hashset_destroy(&set);
  *p_size = kev_addrarray_size(&array);
  return (KevSymbol**)array.begin;
}

void kev_lr_util_symbol_array_partition(KevSymbol** array, size_t size) {
  KevSymbol** left = array;
  KevSymbol** right = array + size - 1;
  while (left < right) {
    while (left < right && (*left)->kind == KEV_LR_SYMBOL_TERMINAL)
      ++left;
    while (left < right && (*right)->kind == KEV_LR_SYMBOL_NONTERMINAL)
      --right;
    if (left >= right) break;
    KevSymbol* tmp = *left;
    *left = *right;
    *right = tmp;
  }
}

KevBitSet** kev_lr_util_compute_follows(KevSymbol** symbols, KevBitSet** firsts, size_t symbol_no, size_t terminal_no, KevSymbol* start, KevSymbol** ends, size_t ends_no) {
  KevBitSet curr_follow;
  if (!kev_bitset_init(&curr_follow, terminal_no + 1))
    return false;
  KevBitSet** follows = (KevBitSet**)malloc(sizeof (KevBitSet*) * symbol_no);
  if (!follows) {
    kev_bitset_destroy(&curr_follow);
    return NULL;
  }
  /* initialize */
  for (size_t i = 0; i < terminal_no; ++i)
    follows[i] = NULL;
  for (size_t i = terminal_no; i < symbol_no; ++i) {
    if (!(follows[i] = kev_bitset_create(terminal_no + 1))) {
      for (size_t j = terminal_no; j < i; ++j)
        kev_bitset_delete(follows[j]);
      free(follows);
      kev_bitset_destroy(&curr_follow);
      return NULL;
    }
  }
  for (size_t i = 0; i < ends_no; ++i)
    kev_bitset_set(follows[start->tmp_id], ends[i]->tmp_id);

  bool loop = true;
  size_t epsilon = terminal_no;
  while (loop) {
    loop = false;
    for (size_t i = 0; i < symbol_no; ++i) {
      KevSymbol* head = symbols[i];
      KevBitSet* head_follow = follows[head->tmp_id];
      for (KevRuleNode* rulenode = head->rules; rulenode; rulenode = rulenode->next) {
        KevSymbol** body = rulenode->rule->body;
        size_t len = rulenode->rule->bodylen;
        if (len == 0) continue;
        size_t i = len;
        kev_bitset_assign(&curr_follow, head_follow);
        do {
          --i;
          KevSymbol* symbol = body[i];
          if (symbol->kind == KEV_LR_SYMBOL_TERMINAL) {
            kev_bitset_make_empty(&curr_follow);
            kev_bitset_set(&curr_follow, symbol->tmp_id);
          } else {
            if (kev_bitset_changed_after_shrinking_union(follows[symbol->tmp_id], &curr_follow))
              loop = true;
            if (kev_bitset_has_element(firsts[symbol->tmp_id], epsilon)) {
              kev_bitset_union(&curr_follow, firsts[symbol->tmp_id]);
              kev_bitset_clear(&curr_follow, epsilon);
            } else {
              kev_bitset_assign(&curr_follow, firsts[symbol->tmp_id]);
            }
          }
        } while (i != 0);
      }
    }
  }

  kev_bitset_destroy(&curr_follow);
  return follows;
}

size_t kev_lr_util_symbol_max_id(KevLRCollection* collec) {
  size_t max_id = 0;
  KevSymbol** symbols = kev_lr_collection_get_symbols(collec);
  size_t symbol_no = collec->symbol_no;
  for (size_t i = 0; i < symbol_no; ++i) {
    if (max_id < symbols[i]->id)
      max_id = symbols[i]->id;
  }
  return max_id;
}
