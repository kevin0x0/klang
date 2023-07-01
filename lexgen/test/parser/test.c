#include "lexgen/include/finite_automaton/finite_automaton.h"
#include "lexgen/include/parser/regex.h"
#include <stdio.h>

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

void print_acc_mapping_array(FILE* out, KevFA* dfa, uint64_t* array) {
  if (!dfa || !array) return;
  KevGraphNode* accept_node = dfa->accept_states;
  uint64_t i = 0;
  while (accept_node) {
    fprintf(out, "accept node %llu is from %lluth nfa\n", accept_node->id, array[i++] + 1);
    accept_node = accept_node->next;
  }
}

int main(int argc, char** argv) {
  while (true) {
    KevFA* nfa_a = kev_nfa_create('a');
    kev_strfamap_update("mynfa", nfa_a);
    KevFA* nfa = kev_regex_parse(argv[1]);
    if (nfa) {
      kev_fa_state_assign_id(nfa, 0);
      print_nfa(stdout, nfa);
      KevFA* array[2] = { nfa, NULL };
      KevFA* dfa = kev_nfa_to_dfa(array, NULL);
      KevFA* min_dfa = kev_dfa_minimization(dfa, NULL);
      kev_fa_state_assign_id(min_dfa, 0);
      print_dfa(stdout, min_dfa);
      kev_fa_delete(nfa);
      kev_fa_delete(min_dfa);
      kev_fa_delete(dfa);
    } else {
      printf("%s\n", kev_regex_get_info());
    }
    kev_fa_delete(nfa_a);
    kev_regex_destroy_named_nfa();
  }
  return 0;
  
}
