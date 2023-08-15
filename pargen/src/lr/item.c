#include "pargen/include/lr/item.h"

void kev_lr_itemset_delete(KevItemSet* itemset) {
  KevKernelItem* item = itemset->items;
  while (item) {
    KevKernelItem* tmp = item->next;
    kev_lr_kernel_item_delete(item);
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

void kev_lr_itemset_add_item(KevItemSet* itemset, KevKernelItem* item) {
  if (!itemset->items || kev_lr_item_less_than(item, itemset->items)) {
    item->next = itemset->items;
    itemset->items = item;
    return;
  }

  KevKernelItem* items = itemset->items;
  while (items->next && !kev_lr_item_less_than(item, items->next))
    items = items->next;
  item->next = items->next;
  items->next = item;
}
