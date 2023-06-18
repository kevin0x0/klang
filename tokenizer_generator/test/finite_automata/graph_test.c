#include "tokenizer_generator/include/finite_automata/dfa.h"
#include "tokenizer_generator/include/finite_automata/graph.h"
#include "tokenizer_generator/include/finite_automata/nfa.h"
#include "tokenizer_generator/include/finite_automata/hashmap/address_map.h"
#include "tokenizer_generator/include/finite_automata/hashmap/intset_map.h"
#include "tokenizer_generator/include/finite_automata/hashmap/setint_map.h"
#include "tokenizer_generator/include/finite_automata/bitset/bitset.h"
#include "tokenizer_generator/include/finite_automata/array/node_array.h"
#include "tokenizer_generator/include/finite_automata/queue/int_queue.h"
#include "tokenizer_generator/include/finite_automata/nfa_to_dfa.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>


void print_graph(FILE* out, KevGraph* graph) {
  if (graph->head == NULL) return;

  KevGraphNode* node = graph->head;
  while (node != NULL) {
    KevGraphEdge* edge = node->edges;
    fprintf(out, "%llu: ", node->attr);
    while (edge) {
      char ch = edge->attr;
      if (ch == KEV_NFA_SYMBOL_EMPTY)
        fprintf(out, " -- -> %llu ", edge->node->attr);
      else if (ch == KEV_NFA_SYMBOL_EPSILON)
        fprintf(out, " ----> %llu ", edge->node->attr);
      else
        fprintf(out, " --%c-> %llu ", ch, edge->node->attr);
      edge = edge->next;
    }
    fprintf(out, "\n");
    node = node->next;
  }
}

void print_nfa(FILE* out, KevNFA* nfa) {
  if (!nfa || !nfa->start_state || !nfa->accept_state) return;
  fprintf(out, "start: %llu\n", nfa->start_state->attr);
  fprintf(out, "accept: %llu\n", nfa->accept_state->attr);
  print_graph(out, &nfa->transition);
}

void print_dfa(FILE* out, KevDFA* dfa) {
  if (!dfa || !dfa->start_state) return;
  fprintf(out, "start: %llu\n", dfa->start_state->attr);
  KevGraphNode* accept_node = dfa->accept_states;
  while (accept_node) {
    fprintf(out, "accept: %llu\n", accept_node->attr);
    accept_node = accept_node->next;
  }
  print_graph(out, &dfa->transition);
}

void print_bitset(FILE* out, KevBitSet* bitset) {
  if (!bitset) return;

  for (uint64_t i = 0; i < bitset->length; ++i) {
    fprintf(out, "%016llX", bitset->bits[i]);
  }
  fprintf(out, "\n");
}

int main(void) {
  while (true) {
    KevNFA* nfa1 = kev_nfa_create('a');
    kev_nfa_kleene(nfa1);
    KevNFA* nfa_array[2] = { nfa1, NULL };
    KevDFA* dfa = kev_nfa_to_dfa(nfa_array, NULL);
    kev_nfa_state_labeling((KevNFA*)dfa, 0);
    //print_dfa(stdout, dfa);
    kev_dfa_delete(dfa);
    kev_nfa_delete(nfa1);
  }


  while (true) {
    KevSetIntMap map;
    kev_setint_map_init(&map, 8);
    KevBitSet* set1 = kev_bitset_create(8);
    KevBitSet* set2 = kev_bitset_create(8);
    kev_bitset_set(set1, 0);
    kev_bitset_set(set2, 1);
    kev_bitset_set(set2, 164);
    kev_setint_map_insert(&map, set1, 0);
    kev_setint_map_insert(&map, set2, 1);
    KevSetIntMapNode* node = kev_setint_map_search(&map, set1);
    printf("%llu ", node->value);
    node = kev_setint_map_search(&map, set2);
    printf("%llu ", node->value);
    kev_setint_map_destroy(&map);
    kev_bitset_delete(set1);
    kev_bitset_delete(set2);
    printf("\n");
  }
  

  while (true) {
    KevIntQueue queue;
    kev_intqueue_init(&queue);
    for (uint64_t i = 0; i < 144; ++i) {
      kev_intqueue_insert(&queue, i);
    }

    for (uint64_t i = 0; i < 72; ++i) {
      printf("%llu ", kev_intqueue_pop(&queue));
    }

    for (uint64_t i = 144; i < 180; ++i) {
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
    for (uint64_t i = 0; i < 144; ++i) {
      KevGraphNode* node = kev_graphnode_create(i);
      kev_nodearray_push_back(&array, node);
    }

    for (uint64_t i = 0; i < kev_nodearray_size(&array); ++i) {
      printf("%llu ", kev_nodearray_visit(&array, i)->attr);
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

    uint64_t i = 0;
    uint64_t next = 0;
    while ((next = kev_bitset_iterate_next(bitset, i)) != i) {
      printf("%llu ", i = next);
    }
    putchar('\n');
    kev_bitset_delete(bitset);
    kev_bitset_delete(bitset1);
  }

  while (true) {
    KevNFA* nfa1 = kev_nfa_create('b');
    KevNFA* nfa2 = kev_nfa_create('a');
    kev_nfa_alternation(nfa1, nfa2);
    kev_nfa_state_labeling(nfa1, 0);
    print_nfa(stdout, nfa1);
    putchar('\n');
    putchar('\n');
    kev_nfa_delete(nfa1);
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
