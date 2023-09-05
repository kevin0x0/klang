#include "kevlr/include/conflict_handle.h"
#include "kevlr/include/hashmap/priority_map.h"

static KevLRConflictHandler struct_reducing = { .object = NULL, .callback = kev_lr_confhandler_reducing_callback };
KevLRConflictHandler* kev_lr_confhandler_reducing = &struct_reducing;
static KevLRConflictHandler struct_shifting = { .object = NULL, .callback = kev_lr_confhandler_shifting_callback };
KevLRConflictHandler* kev_lr_confhandler_shifting = &struct_shifting;

bool kev_lr_confhandler_priority_callback(void* object, KevLRConflict* conflict, KevLRCollection* collec) {
  /* this handler can not handle reducing-reducing conflict */
  if (kev_lr_conflict_RR(conflict)) return false;
  KevPrioMap* priomap = (KevPrioMap*)object;
  KevSymbol* postfix_operator = conflict->symbol;
  /* find the priority of the postfix operator */
  KevPrioMapNode* mapnode = kev_priomap_search(priomap, postfix_operator, KEV_LR_SYMBOL_POS_POSTFIX);
  if (!mapnode) return false;
  size_t postfix_prio = mapnode->priority;
  /* find the priority of the operator in the reduced rule(not after the rule). */
  KevRule* rule = conflict->itemset->items->rule;
  KevSymbol** rulebody = rule->body;
  size_t rulelen = rule->bodylen;
  for (size_t i = 0; i < rulelen; ++i) {
    KevPrioMapNode* mapnode = kev_priomap_search(priomap, rulebody[i], (KevPrioPos)i);
    if (!mapnode) continue;
    /* found the priority of operator in rule. */
    size_t inrule_prio = mapnode->priority;
    if (inrule_prio > postfix_prio) { /* choose reducing */
      kev_lr_conflict_set_reducing(conflict, conflict->itemset->items);
      return true;
    } else if (inrule_prio < postfix_prio) {  /* choose shifting */
      kev_lr_conflict_set_shifting(conflict);
      return true;
    } else {  /* This handler can not handle the case when operators have the same priority. */
      return false;
    }
  }
  return false;
}
