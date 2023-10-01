#include "kevlr/include/itemset.h"

#include <stdlib.h>

static KevBitSet* kev_lr_get_kernel_item_follows(KevItem* kitem, KevBitSet** firsts, size_t epsilon);
static KevBitSet* kev_lr_get_non_kernel_item_follows(KevRule* rule, KevBitSet* lookahead, KevBitSet** firsts, size_t epsilon);
static bool kev_lr_closure_propagate(KevItemSetClosure* closure, size_t epsilon);

void kev_lr_itemset_delete(KevItemSet* itemset) {
  KevItem* item = itemset->items;
  while (item) {
    KevItem* tmp = item->next;
    kev_lr_item_delete(item);
    item = tmp;
  }
  KevItemSetGoto* go_to = itemset->gotos;
  while (go_to) {
    KevItemSetGoto* tmp = go_to->next;
    free(go_to);
    go_to = tmp;
  }
  kev_itemset_pool_deallocate(itemset);
}

void kev_lr_itemset_add_item(KevItemSet* itemset, KevItem* item) {
  if (!itemset->items || kev_lr_item_less_than(item, itemset->items)) {
    item->next = itemset->items;
    itemset->items = item;
    return;
  }

  KevItem* items = itemset->items;
  while (items->next && !kev_lr_item_less_than(item, items->next))
    items = items->next;
  item->next = items->next;
  items->next = item;
}

void kev_lr_closure_delete(KevItemSetClosure* closure) {
  kev_lr_closure_destroy(closure);
  free(closure);
}

void kev_lr_closure_destroy(KevItemSetClosure* closure) {
  if (!closure) return;
  KevAddrArray* symbols = closure->symbols;
  size_t size = kev_addrarray_size(symbols);
  for (size_t i = 0; i < size; ++i) {
    size_t index = ((KevSymbol*)kev_addrarray_visit(symbols, i))->index;
    kev_bitset_delete(closure->lookaheads[index]);
  }
  free(closure->lookaheads);
  kev_addrarray_delete(closure->symbols);
  closure->symbols = NULL;
  closure->lookaheads = NULL;
}

void kev_lr_closure_make_empty(KevItemSetClosure* closure) {
  size_t size = kev_addrarray_size(closure->symbols);
  KevBitSet** las = closure->lookaheads;
  KevAddrArray* symbols = closure->symbols;
  for (size_t i = 0; i < size; ++i) {
    size_t index = ((KevSymbol*)kev_addrarray_visit(closure->symbols, i))->index;
    kev_bitset_delete(las[index]);
    las[index] = NULL;
  }
  kev_addrarray_make_empty(symbols);
}

bool kev_lr_closure_init(KevItemSetClosure* closure, size_t symbol_no) {
  KevAddrArray* symbols = kev_addrarray_create();
  KevBitSet** las = (KevBitSet**)malloc(sizeof (KevBitSet*) * symbol_no);
  if (!symbols || !las) {
    free(symbols);
    free(las);
    return false;
  }
  for (size_t i = 0; i < symbol_no; ++i)
    las[i] = NULL;
  closure->lookaheads = las;
  closure->symbols = symbols;
  return true;
}

KevItemSetClosure* kev_lr_closure_create(size_t symbol_no) {
  KevItemSetClosure* closure = (KevItemSetClosure*)malloc(sizeof (KevItemSetClosure));
  if (!closure || !kev_lr_closure_init(closure, symbol_no)) {
    free(closure);
    return NULL;
  }
  return closure;
}

bool kev_lr_closure_make(KevItemSetClosure* closure, KevItemSet* itemset, KevBitSet** firsts, size_t epsilon) {
  KevAddrArray* symbols = closure->symbols;
  KevBitSet** las = closure->lookaheads;
  KevItem* kitem = itemset->items;
  for (; kitem; kitem = kitem->next) {
    if (kitem->dot == kitem->rule->bodylen)
      continue;
    KevSymbol* symbol = kitem->rule->body[kitem->dot];
    if (symbol->kind == KEV_LR_TERMINAL)
      continue;
    KevBitSet* la = kev_lr_get_kernel_item_follows(kitem, firsts, epsilon);
    if (!la) return false;
    size_t index = symbol->index;
    if (las[index]) {
      if (!kev_bitset_union(las[index], la)) {
        kev_bitset_delete(la);
        return false;
      }
      kev_bitset_delete(la);
    } else {
      if (!kev_addrarray_push_back(symbols, symbol)) {
        kev_bitset_delete(la);
        return false;
      }
      las[index] = la;
    }
  }

  for (size_t i = 0; i < kev_addrarray_size(symbols); ++i) {
    KevSymbol* head = (KevSymbol*)kev_addrarray_visit(symbols, i);
    size_t head_index = head->index;
    for (KevRuleNode* node = head->rules; node; node = node->next) {
      KevRule* rule = node->rule;
      if (rule->bodylen == 0) continue;
      KevSymbol* symbol = rule->body[0];
      if (symbol->kind == KEV_LR_TERMINAL)
        continue;
      KevBitSet* la = kev_lr_get_non_kernel_item_follows(rule, las[head_index], firsts, epsilon);
      if (!la) return false;
      size_t index = symbol->index;
      if (las[index]) {
        if (!kev_bitset_union(las[index], la)) {
          kev_bitset_delete(la);
          return false;
        }
        kev_bitset_delete(la);
      } else {
        if (!kev_addrarray_push_back(symbols, symbol)) {
          kev_bitset_delete(la);
          return false;
        }
        las[index] = la;
      }
    }
  }

  return kev_lr_closure_propagate(closure, epsilon);
}

static bool kev_lr_closure_propagate(KevItemSetClosure* closure, size_t epsilon) {
  KevAddrArray* symbols = closure->symbols;
  KevBitSet** las = closure->lookaheads;
  size_t symbol_array_size = kev_addrarray_size(symbols);
  bool not_done = true;
  while (not_done) {
    not_done = false;
    for (size_t i = 0; i < symbol_array_size; ++i) {
      KevSymbol* symbol = (KevSymbol*)kev_addrarray_visit(symbols, i);
      KevRuleNode* node = symbol->rules;
      for (; node; node = node->next) {
        size_t len = node->rule->bodylen;
        KevSymbol** body = node->rule->body;
        if (len == 0 || body[0]->kind == KEV_LR_TERMINAL)
          continue;
        size_t i = 1;
        for (; i < len; ++i) {
          if (body[i]->kind == KEV_LR_TERMINAL || 
              !kev_bitset_has_element(las[body[i]->index], epsilon))
            break;
        }
        if (i != len) continue;
        if (!kev_bitset_is_subset(las[symbol->index], las[body[0]->index])) {
          not_done = true;
          if (!kev_bitset_union(las[body[0]->index], las[symbol->index]))
            return false;
        }
      }
    }
  }
  return true;
}

static KevBitSet* kev_lr_get_kernel_item_follows(KevItem* kitem, KevBitSet** firsts, size_t epsilon) {
  size_t len = kitem->rule->bodylen;
  KevSymbol** rulebody = kitem->rule->body;
  KevBitSet* follows = kev_bitset_create(epsilon + 3);
  if (!follows) return NULL;
  for (size_t i = kitem->dot + 1; i < len; ++i) {
    if (rulebody[i]->kind == KEV_LR_TERMINAL) {
      kev_bitset_set(follows, rulebody[i]->index);
      return follows;
    }
    KevBitSet* set = firsts[rulebody[i]->index];
    if (!kev_bitset_union(follows, set)) {
      kev_bitset_delete(follows);
      return NULL;
    }
    if (!kev_bitset_has_element(set, epsilon)) {
      kev_bitset_clear(follows, epsilon);
      return follows;
    }
  }
  kev_bitset_clear(follows, epsilon);
  if (!kev_bitset_union(follows, kitem->lookahead)) {
    kev_bitset_delete(follows);
    return NULL;
  }
  return follows;
}

static KevBitSet* kev_lr_get_non_kernel_item_follows(KevRule* rule, KevBitSet* lookahead, KevBitSet** firsts, size_t epsilon) {
  size_t len = rule->bodylen;
  KevSymbol** rulebody = rule->body;
  KevBitSet* follows = kev_bitset_create(epsilon + 3);
  if (!follows) return NULL;
  for (size_t i = 1; i < len; ++i) {
    if (rulebody[i]->kind == KEV_LR_TERMINAL) {
      kev_bitset_set(follows, rulebody[i]->index);
      return follows;
    }
    KevBitSet* set = firsts[rulebody[i]->index];
    if (!kev_bitset_union(follows, set)) {
      kev_bitset_delete(follows);
      return NULL;
    }
    if (!kev_bitset_has_element(set, epsilon)) {
      kev_bitset_clear(follows, epsilon);
      return follows;
    }
  }
  kev_bitset_clear(follows, epsilon);
  if (!kev_bitset_union(follows, lookahead)) {
    kev_bitset_delete(follows);
    return NULL;
  }
  return follows;
}

