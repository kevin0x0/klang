#ifndef KEVCC_KEVFA_INCLUDE_FINITE_AUTOMATON_H
#define KEVCC_KEVFA_INCLUDE_FINITE_AUTOMATON_H

#include "kevfa/include/graph.h"
#include "utils/include/general/global_def.h"

typedef KevGraphNodeId KevStateId;
typedef KevGraphEdgeAttr KevNFAChar;

/* This structure is used to represent NFA and DFA.
 * 'transition is a linked list of states.
 *
 * For NFA, there is only one starting state and accepting state,
 * which are pointed by 'start_state' and 'accept_states' separately.
 * The starting state only has outgoing edges, and the accepting states
 * only have incoming edges. starting state must be the fisrt node in
 * the 'transition'.
 *
 * For DFA, there is only one starting state, and zero or more accepting
 * states. Starting state can alse be an accepting state. All the
 * accepting states should be placed in the tail of 'transition'. */
typedef struct tagKevFA {
  KevGraph transition;
  KevGraphNode* start_state;
  KevGraphNodeList* accept_states;
} KevFA;

bool kev_nfa_init(KevFA* fa, KevNFAChar character);
bool kev_nfa_init_empty(KevFA* fa);
bool kev_nfa_init_epsilon(KevFA* fa);
bool kev_fa_init_copy(KevFA* fa, KevFA* src);
bool kev_fa_init_move(KevFA* fa, KevFA* src);
/* Do not check parameters. You should aware of what you are doing. */
bool kev_fa_init_set(KevFA* fa, KevGraphNodeList* state_list, KevGraphNode* start, KevGraphNode* accept);
void kev_fa_destroy(KevFA* fa);
KevFA* kev_nfa_create(KevNFAChar character);
KevFA* kev_nfa_create_epsilon(void);
KevFA* kev_nfa_create_empty(void);
/* this remain validity of NFAs */
KevFA* kev_fa_create_copy(KevFA* src);
/* this remain validity of NFAs */
KevFA* kev_fa_create_move(KevFA* src);
/* Do not check parameters. You should aware of what you are doing. */
KevFA* kev_fa_create_set(KevGraphNodeList* state_list, KevGraphNode* start, KevGraphNode* accept);
void kev_fa_delete(KevFA* fa);

static inline bool kev_nfa_add_transition(KevFA* nfa, KevNFAChar character);
static inline bool kev_nfa_add_epsilon(KevFA* nfa);
size_t kev_fa_state_assign_id(KevFA* fa, KevStateId start_id);
/* Do concatenation between 'dest' and 'src'. 
 * The result is in 'dest', 'src' would be set to empty */
bool kev_nfa_concatenation(KevFA* dest, KevFA* src);
/* Do alternation between 'dest' and 'src'. 
 * The result is in 'dest', 'src' would be set to empty */
bool kev_nfa_alternation(KevFA* dest, KevFA* src);
/* Do kleene closure between 'dest' and 'src'. */
static inline bool kev_nfa_kleene(KevFA* nfa);
/* Do positive closure between 'dest' and 'src'. */
bool kev_nfa_positive(KevFA* nfa);
/* Given an array of pointers to NFAs, use the subset construction algorithm
 * to convert the union of these NFAs into a DFA and return a pointer to the
 * resulting DFA. The array of NFA pointers should terminate with a NULL pointer.
 * If the "p_accept_state_mapping_array" parameter is not NULL, it will point
 * to an array that records which NFA each accepting state in the DFA corresponds to.
 * For example, if this array is named "acc_state_mapping" and acc_state_mapping[2] = 0,
 * it means that the third accepting state in the DFA corresponds to the first
 * NFA in the input array of NFA pointers. */
KevFA* kev_nfa_to_dfa(KevFA** nfa_array, size_t** p_accept_state_mapping_array);
KevFA* kev_dfa_minimization(KevFA* dfa, size_t* accept_state_mapping);
static inline size_t kev_fa_state_number(KevFA* dfa);
static inline size_t kev_dfa_accept_state_number(KevFA* dfa);
static inline size_t kev_dfa_non_accept_state_number(KevFA* dfa);
static inline KevGraphNode* kev_fa_get_start_state(KevFA* fa);
static inline KevGraphNode* kev_fa_get_accept_state(KevFA* fa);
static inline KevGraphNode* kev_fa_get_states(KevFA* fa);

static inline bool kev_nfa_add_transition(KevFA* nfa, KevNFAChar character) {
  return kev_graphnode_connect(nfa->start_state, nfa->accept_states, character);
}

static inline bool kev_nfa_add_epsilon(KevFA* nfa) {
  return kev_graphnode_connect_epsilon(nfa->start_state, nfa->accept_states);
}

static inline bool kev_nfa_kleene(KevFA* nfa) {
  return kev_nfa_positive(nfa) &&
         kev_graphnode_connect_epsilon(nfa->start_state, nfa->accept_states);
}

static inline KevGraphNode* kev_fa_get_start_state(KevFA* fa) {
  return fa->start_state;
}

static inline KevGraphNode* kev_fa_get_accept_state(KevFA* fa) {
  return fa->accept_states;
}

static inline KevGraphNode* kev_fa_get_states(KevFA* fa) {
  return kev_graph_get_nodes(&fa->transition);
}

static inline size_t kev_fa_state_number(KevFA* dfa) {
  uint64_t count = 0;
  for (KevGraphNode* node = kev_fa_get_states(dfa); node; node = node->next)
    ++count;
  return count;
}

static inline size_t kev_dfa_accept_state_number(KevFA* dfa) {
  uint64_t count = 0;
  for (KevGraphNode* node = dfa->accept_states; node; node = node->next)
    ++count;
  return count;
}

static inline size_t kev_dfa_non_accept_state_number(KevFA* dfa) {
  KevGraphNode* acc_node = dfa->accept_states;
  uint64_t count = 0;
  for (KevGraphNode* node = kev_fa_get_states(dfa); node != acc_node; node = node->next)
    ++count;
  return count;
}

#endif
