#include "kevlr/include/itemset.h"

#include <stdlib.h>
#include <string.h>

static KBitSet* klr_get_kernel_item_follows(KlrItem* kitem, KBitSet** firsts, size_t epsilon);
static KBitSet* klr_get_non_kernel_item_follows(KlrRule* rule, KBitSet* lookahead, KBitSet** firsts, size_t epsilon);
static bool klr_closure_propagate(KlrItemSetClosure* closure, size_t epsilon);

void klr_itemset_delete(KlrItemSet* itemset) {
  KlrItem* item = itemset->items;
  while (item) {
    KlrItem* tmp = item->next;
    klr_item_delete(item);
    item = tmp;
  }
  KlrItemSetTransition* trans = itemset->trans;
  while (trans) {
    KlrItemSetTransition* tmp = trans->next;
    free(trans);
    trans = tmp;
  }
  klr_itemset_pool_deallocate(itemset);
}

void klr_itemset_add_item(KlrItemSet* itemset, KlrItem* item) {
  if (!itemset->items || klr_item_less_than(item, itemset->items)) {
    item->next = itemset->items;
    itemset->items = item;
    return;
  }

  KlrItem* items = itemset->items;
  while (items->next && !klr_item_less_than(item, items->next))
    items = items->next;
  item->next = items->next;
  items->next = item;
}

void klr_closure_delete(KlrItemSetClosure* closure) {
  klr_closure_destroy(closure);
  free(closure);
}

void klr_closure_destroy(KlrItemSetClosure* closure) {
  if (k_unlikely(!closure)) return;
  KArray* symbols = closure->symbols;
  size_t size = karray_size(symbols);
  for (size_t i = 0; i < size; ++i) {
    size_t index = ((KlrSymbol*)karray_access(symbols, i))->index;
    kbitset_delete(closure->lookaheads[index]);
  }
  free(closure->lookaheads);
  karray_delete(closure->symbols);
  closure->symbols = NULL;
  closure->lookaheads = NULL;
}

void klr_closure_make_empty(KlrItemSetClosure* closure) {
  size_t size = karray_size(closure->symbols);
  KBitSet** las = closure->lookaheads;
  KArray* symbols = closure->symbols;
  for (size_t i = 0; i < size; ++i) {
    size_t index = ((KlrSymbol*)karray_access(closure->symbols, i))->index;
    kbitset_delete(las[index]);
    las[index] = NULL;
  }
  karray_make_empty(symbols);
}

bool klr_closure_init(KlrItemSetClosure* closure, size_t symbol_no) {
  KArray* symbols = karray_create();
  KBitSet** las = (KBitSet**)malloc(sizeof (KBitSet*) * symbol_no);
  if (k_unlikely(!symbols || !las)) {
    free(symbols);
    free(las);
    return false;
  }
  memset(las, 0, sizeof (KBitSet*) * symbol_no);
  closure->lookaheads = las;
  closure->symbols = symbols;
  return true;
}

KlrItemSetClosure* klr_closure_create(size_t symbol_no) {
  KlrItemSetClosure* closure = (KlrItemSetClosure*)malloc(sizeof (KlrItemSetClosure));
  if (k_unlikely(!closure || !klr_closure_init(closure, symbol_no))) {
    free(closure);
    return NULL;
  }
  return closure;
}

bool klr_closure_make(KlrItemSetClosure* closure, KlrItemSet* itemset, KBitSet** firsts, size_t epsilon) {
  KArray* symbols = closure->symbols;
  KBitSet** las = closure->lookaheads;
  KlrItem* kitem = itemset->items;
  for (; kitem; kitem = kitem->next) {
    if (kitem->dot == kitem->rule->bodylen)
      continue;
    KlrSymbol* symbol = kitem->rule->body[kitem->dot];
    if (symbol->kind == KLR_TERMINAL)
      continue;
    KBitSet* la = klr_get_kernel_item_follows(kitem, firsts, epsilon);
    if (k_unlikely(!la)) return false;
    size_t index = symbol->index;
    if (las[index]) {
      if (k_unlikely(!kbitset_union(las[index], la))) {
        kbitset_delete(la);
        return false;
      }
      kbitset_delete(la);
    } else {
      if (k_unlikely(!karray_push_back(symbols, symbol))) {
        kbitset_delete(la);
        return false;
      }
      las[index] = la;
    }
  }

  for (size_t i = 0; i < karray_size(symbols); ++i) {
    KlrSymbol* head = (KlrSymbol*)karray_access(symbols, i);
    size_t head_index = head->index;
    for (KlrRuleNode* node = head->rules; node; node = node->next) {
      KlrRule* rule = node->rule;
      if (rule->bodylen == 0) continue;
      KlrSymbol* symbol = rule->body[0];
      if (symbol->kind == KLR_TERMINAL)
        continue;
      KBitSet* la = klr_get_non_kernel_item_follows(rule, las[head_index], firsts, epsilon);
      if (k_unlikely(!la)) return false;
      size_t index = symbol->index;
      if (las[index]) {
        if (k_unlikely(!kbitset_union(las[index], la))) {
          kbitset_delete(la);
          return false;
        }
        kbitset_delete(la);
      } else {
        if (k_unlikely(!karray_push_back(symbols, symbol))) {
          kbitset_delete(la);
          return false;
        }
        las[index] = la;
      }
    }
  }

  return klr_closure_propagate(closure, epsilon);
}

static bool klr_closure_propagate(KlrItemSetClosure* closure, size_t epsilon) {
  KArray* symbols = closure->symbols;
  KBitSet** las = closure->lookaheads;
  size_t symbol_array_size = karray_size(symbols);
  bool not_done = true;
  while (not_done) {
    not_done = false;
    for (size_t i = 0; i < symbol_array_size; ++i) {
      KlrSymbol* symbol = (KlrSymbol*)karray_access(symbols, i);
      KlrRuleNode* node = symbol->rules;
      for (; node; node = node->next) {
        size_t len = node->rule->bodylen;
        KlrSymbol** body = node->rule->body;
        if (len == 0 || body[0]->kind == KLR_TERMINAL)
          continue;
        size_t i = 1;
        for (; i < len; ++i) {
          if (body[i]->kind == KLR_TERMINAL || 
              !kbitset_has_element(las[body[i]->index], epsilon))
            break;
        }
        if (i != len) continue;
        if (!kbitset_is_subset(las[symbol->index], las[body[0]->index])) {
          not_done = true;
          if (k_unlikely(!kbitset_union(las[body[0]->index], las[symbol->index])))
            return false;
        }
      }
    }
  }
  return true;
}

static KBitSet* klr_get_kernel_item_follows(KlrItem* kitem, KBitSet** firsts, size_t epsilon) {
  size_t len = kitem->rule->bodylen;
  KlrSymbol** rulebody = kitem->rule->body;
  KBitSet* follows = kbitset_create(epsilon + 3);
  if (!follows) return NULL;
  for (size_t i = kitem->dot + 1; i < len; ++i) {
    if (rulebody[i]->kind == KLR_TERMINAL) {
      kbitset_set(follows, rulebody[i]->index);
      return follows;
    }
    KBitSet* set = firsts[rulebody[i]->index];
    if (k_unlikely(!kbitset_union(follows, set))) {
      kbitset_delete(follows);
      return NULL;
    }
    if (!kbitset_has_element(set, epsilon)) {
      kbitset_clear(follows, epsilon);
      return follows;
    }
  }
  kbitset_clear(follows, epsilon);
  if (k_unlikely(!kbitset_union(follows, kitem->lookahead))) {
    kbitset_delete(follows);
    return NULL;
  }
  return follows;
}

static KBitSet* klr_get_non_kernel_item_follows(KlrRule* rule, KBitSet* lookahead, KBitSet** firsts, size_t epsilon) {
  size_t len = rule->bodylen;
  KlrSymbol** rulebody = rule->body;
  KBitSet* follows = kbitset_create(epsilon + 3);
  if (!follows) return NULL;
  for (size_t i = 1; i < len; ++i) {
    if (rulebody[i]->kind == KLR_TERMINAL) {
      kbitset_set(follows, rulebody[i]->index);
      return follows;
    }
    KBitSet* set = firsts[rulebody[i]->index];
    if (k_unlikely(!kbitset_union(follows, set))) {
      kbitset_delete(follows);
      return NULL;
    }
    if (!kbitset_has_element(set, epsilon)) {
      kbitset_clear(follows, epsilon);
      return follows;
    }
  }
  kbitset_clear(follows, epsilon);
  if (k_unlikely(!kbitset_union(follows, lookahead))) {
    kbitset_delete(follows);
    return NULL;
  }
  return follows;
}

