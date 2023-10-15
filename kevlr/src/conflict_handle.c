#include "kevlr/include/conflict_handle.h"
#include "kevlr/include/hashmap/priority_map.h"

static KlrConflictHandler struct_reducing = { .object = NULL, .callback = klr_confhandler_reducing_callback };
KlrConflictHandler* klr_confhandler_reducing = &struct_reducing;
static KlrConflictHandler struct_shifting = { .object = NULL, .callback = klr_confhandler_shifting_callback };
KlrConflictHandler* klr_confhandler_shifting = &struct_shifting;

bool klr_confhandler_priority_callback(void* object, KlrConflict* conflict, KlrCollection* collec) {
  /* this handler callback can not handle reducing-reducing conflict */
  if (klr_conflict_RR(conflict)) return false;
  KlrPrioMap* priomap = (KlrPrioMap*)object;
  KlrSymbol* postfix_operator = klr_conflict_get_symbol(conflict);
  /* find the priority of the operator after the rule */
  KlrPrioMapNode* mapnode = klr_priomap_search(priomap, postfix_operator, KEV_LR_PRIOPOS_POSTFIX);
  if (!mapnode) return false;
  size_t postfix_prio = mapnode->priority;
  /* find the priority of the operator in the reduced rule(not after the rule). */
  KlrItem* item = klr_itemset_iter_begin(klr_conflict_get_conflict_items(conflict));
  KlrRule* rule = klr_item_get_rule(item);
  KlrSymbol** rulebody = klr_rule_get_body(rule);
  size_t rulelen = klr_rule_get_bodylen(rule);
  for (size_t i = 0; i < rulelen; ++i) {
    KlrPrioMapNode* mapnode = klr_priomap_search(priomap, rulebody[i], (KlrPrioPos)i);
    if (!mapnode) continue;
    /* found the priority of operator in rule. */
    size_t inrule_prio = mapnode->priority;
    if (inrule_prio > postfix_prio) { /* choose reducing */
      klr_conflict_set_reducing(conflict, collec, item);
      return true;
    } else if (inrule_prio < postfix_prio) {  /* choose shifting */
      klr_conflict_set_shifting(conflict);
      return true;
    } else {  /* This handler can not handle the case when operators have the same priority. */
      return false;
    }
  }
  return false;
}
