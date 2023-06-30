#include "tokenizer_generator/include/finite_automaton/finite_automaton.h"
#include "tokenizer_generator/include/finite_automaton/graph.h"
#include "tokenizer_generator/include/finite_automaton/hashmap/address_map.h"
#include "tokenizer_generator/include/finite_automaton/hashmap/intset_map.h"
#include "tokenizer_generator/include/finite_automaton/hashmap/setint_map.h"
#include "tokenizer_generator/include/finite_automaton/set/bitset.h"
#include "tokenizer_generator/include/finite_automaton/array/node_array.h"
#include "tokenizer_generator/include/finite_automaton/queue/int_queue.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>


void print_graph(FILE* out, KevGraph* graph) {
  if (graph->head == NULL) return;

  KevGraphNode* node = graph->head;
  while (node != NULL) {
    KevGraphEdge* edge = node->edges;
    fprintf(out, "%llu: ", node->id);
    while (edge) {
      char ch = edge->attr;
      if (ch == KEV_NFA_SYMBOL_EMPTY)
        fprintf(out, " -- -> %llu ", edge->node->id);
      else if (ch == KEV_NFA_SYMBOL_EPSILON)
        fprintf(out, " ----> %llu ", edge->node->id);
      else
        fprintf(out, " --%c-> %llu ", ch, edge->node->id);
      edge = edge->next;
    }
    fprintf(out, "\n");
    node = node->next;
  }
}

void print_nfa(FILE* out, KevFA* nfa) {
  if (!nfa || !nfa->start_state || !nfa->accept_states) return;
  fprintf(out, "start: %llu\n", nfa->start_state->id);
  fprintf(out, "accept: %llu\n", nfa->accept_states->id);
  print_graph(out, &nfa->transition);
}

void print_dfa(FILE* out, KevFA* dfa) {
  if (!dfa || !dfa->start_state) return;
  fprintf(out, "start: %llu\n", dfa->start_state->id);
  KevGraphNode* accept_node = dfa->accept_states;
  while (accept_node) {
    fprintf(out, "accept: %llu\n", accept_node->id);
    accept_node = accept_node->next;
  }
  print_graph(out, &dfa->transition);
}

void print_bitset(FILE* out, KevBitSet* bitset) {
  if (!bitset) return;

  for (size_t i = 0; i < bitset->length; ++i) {
    fprintf(out, "%016llX", bitset->bits[i]);
  }
  fprintf(out, "\n");
}

void print_acc_mapping_array(FILE* out, KevFA* dfa, size_t* array) {
  if (!dfa || !array) return;
  KevGraphNode* accept_node = dfa->accept_states;
  size_t i = 0;
  while (accept_node) {
    fprintf(out, "accept node %llu is from %lluth nfa\n", accept_node->id, array[i++] + 1);
    accept_node = accept_node->next;
  }
}

int main(void) {
  
  while (false) {
    KevGraphNode* node0 = kev_graphnode_create(0);
    KevGraphNode* node1 = kev_graphnode_create(0);
    KevGraphNode* node2 = kev_graphnode_create(0);
    //kev_graphnode_connect(node1, node2, 'a');
    kev_graphnode_connect(node2, node2, 'a');
    kev_graphnode_connect(node0, node1, 'a');
    node0->next = node1;
    node1->next = node2;
    node2->next = NULL;
    KevFA* dfa = kev_fa_create_set(node0, node0, node2);
    KevFA* min_dfa = /*kev_fa_create_copy(dfa);*/kev_dfa_minimization(dfa, NULL);
    kev_fa_state_assign_id(min_dfa, 0);
    //print_dfa(stdout, min_dfa);
    //putchar('\n');
    kev_fa_delete(dfa);
    kev_fa_delete(min_dfa);
  }


    KevFA* nfa1 = kev_nfa_create('a');
    KevFA* nfa2 = kev_nfa_create('b');
    KevFA* nfa3 = kev_nfa_create('c');
    KevFA* nfa4 = kev_nfa_create('d');
    KevFA* nfa5 = kev_nfa_create('e');
    KevFA* nfa6 = kev_nfa_create('f');
    KevFA* nfa7 = kev_nfa_create('g');
    KevFA* tmp = kev_fa_create_copy(nfa7);
    kev_nfa_positive(nfa7);
    for (int i = 0; i < 10; ++i) {
      KevFA* copy1 = kev_fa_create_copy(nfa1);
      KevFA* copy2 = kev_fa_create_copy(tmp);
      kev_nfa_alternation(copy1, copy2);
      kev_nfa_concatenation(nfa7, copy1);
      kev_fa_delete(copy1);
      kev_fa_delete(copy2);
    }
    kev_fa_delete(tmp);
    size_t* acc_map = NULL;
    KevFA* nfa_array[8] = { nfa1, nfa2, nfa3, nfa4, nfa5, nfa6, nfa7, NULL };
    KevFA* dfa = kev_nfa_to_dfa(nfa_array, &acc_map);
    KevFA* min_dfa = /*kev_fa_create_copy(dfa);*/kev_dfa_minimization(dfa, acc_map);
    kev_fa_state_assign_id(min_dfa, 0);
    //print_dfa(stdout, min_dfa);
    //print_acc_mapping_array(stdout, min_dfa, acc_map);
    //putchar('\n');
    kev_fa_delete(dfa);
    kev_fa_delete(min_dfa);
    kev_fa_delete(nfa1);
    kev_fa_delete(nfa2);
    kev_fa_delete(nfa3);
    kev_fa_delete(nfa4);
    kev_fa_delete(nfa5);
    kev_fa_delete(nfa6);
    kev_fa_delete(nfa7);
    free(acc_map);
    return 0;


  while (true) {
    KevSetIntMap map;
    kev_setintmap_init(&map, 8);
    KevBitSet* set1 = kev_bitset_create(8);
    KevBitSet* set2 = kev_bitset_create(8);
    kev_bitset_set(set1, 0);
    kev_bitset_set(set2, 1);
    kev_bitset_set(set2, 164);
    kev_setintmap_insert(&map, set1, 0);
    kev_setintmap_insert(&map, set2, 1);
    KevSetIntMapNode* node = kev_setintmap_search(&map, set1);
    printf("%llu ", node->value);
    node = kev_setintmap_search(&map, set2);
    printf("%llu ", node->value);
    kev_setintmap_destroy(&map);
    kev_bitset_delete(set1);
    kev_bitset_delete(set2);
    printf("\n");
  }
  

  while (true) {
    KevIntQueue queue;
    kev_intqueue_init(&queue);
    for (size_t i = 0; i < 144; ++i) {
      kev_intqueue_insert(&queue, i);
    }

    for (size_t i = 0; i < 72; ++i) {
      printf("%llu ", kev_intqueue_pop(&queue));
    }

    for (size_t i = 144; i < 180; ++i) {
      kev_intqueue_insert(&queue, i);
    }

    while (!kev_intqueue_empty(&queue)) {
      printf("%llu ", kev_intqueue_pop(&queue));
    }
    printf("\n\n");
    kev_intqueue_destroy(&queue);
  }

  while (true) {
    KevNodeArray array;
    kev_nodearray_init(&array);
    for (size_t i = 0; i < 144; ++i) {
      KevGraphNode* node = kev_graphnode_create(i);
      kev_nodearray_push_back(&array, node);
    }

    for (size_t i = 0; i < kev_nodearray_size(&array); ++i) {
      printf("%llu ", kev_nodearray_visit(&array, i)->id);
      kev_graphnode_delete(kev_nodearray_visit(&array, i));
    }
    printf("\n\n");
    kev_nodearray_destroy(&array);
  }

  while (true) {
    KevBitSet* bitset = kev_bitset_create(32);
    KevBitSet* bitset1 = kev_bitset_create(32);
    kev_bitset_set(bitset1, 175);
    kev_bitset_set(bitset1, 132);
    kev_bitset_set(bitset, 32);
    print_bitset(stdout, bitset);
    print_bitset(stdout, bitset1);
    kev_bitset_union(bitset, bitset1);
    //kev_bitset_intersection(bitset, bitset1);
    //kev_bitset_completion(bitset);
    //print_bitset(stdout, bitset);
    putchar('\n');

    size_t i = 0;
    size_t next = 0;
    while ((next = kev_bitset_iterate_next(bitset, i)) != i) {
      printf("%llu ", i = next);
    }
    putchar('\n');
    kev_bitset_delete(bitset);
    kev_bitset_delete(bitset1);
  }

  while (true) {
    KevFA* nfa1 = kev_nfa_create('b');
    KevFA* nfa2 = kev_nfa_create('a');
    kev_nfa_alternation(nfa1, nfa2);
    kev_fa_state_assign_id(nfa1, 0);
    print_nfa(stdout, nfa1);
    putchar('\n');
    putchar('\n');
    kev_fa_delete(nfa1);
  }

  while (true) {
    KevGraphNode* node1 = kev_graphnode_create(1);
    KevGraphNode* node2 = kev_graphnode_create(2);
    KevGraph *graph = kev_graph_create(node1);
    KevGraph *graph1 = kev_graph_create(node2);
    kev_graphnode_connect(node1, node2, 'f');
    kev_graph_merge(graph, graph1);
    kev_graph_init_copy(graph1, graph);
    print_graph(stdout, graph);
    print_graph(stdout, graph1);
    getchar();
    kev_graph_delete(graph);
    kev_graph_delete(graph1);
  }
  return 0;
}
