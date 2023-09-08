#include "kevfa/include/finite_automaton.h"

int main(int argc, char** argv) {
  KevFA* fa = kev_nfa_create('a');
  KevFA* fa1 = kev_nfa_create('a');
  kev_nfa_alternation(fa, fa1);
  kev_nfa_kleene(fa);
  kev_fa_delete(fa1);
  KevFA* arr[] = { fa, NULL };
  KevFA* dfa = kev_nfa_to_dfa(arr, NULL);
  KevFA* min_dfa = kev_dfa_minimization(dfa, NULL);
  kev_fa_delete(dfa);
  kev_fa_delete(fa);
  kev_fa_delete(min_dfa);
  return 0;
}
