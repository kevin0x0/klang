
OPTIMIZE = -g
CC = gcc
AR = ar


ROOT_PATH = ~/project/kevcc/

CC_GLOBAL_PARAM = -Wall -I $(ROOT_PATH)

LEXGEN_PATH = $(ROOT_PATH)lexgen/
INC_PATH = $(LEXGEN_PATH)include/
SRC_PATH = $(LEXGEN_PATH)src/
BIN_PATH = $(LEXGEN_PATH)bin/
LIB_PATH = $(LEXGEN_PATH)lib/

GEN_INC_PATH = $(INC_PATH)general/

#finite_automata path
FA_INC_PATH = $(INC_PATH)finite_automaton/
FA_PATH = $(SRC_PATH)finite_automaton/

graph_test.exe : graph_test.c $(FA_PATH)graph.c $(FA_PATH)object_pool/fa_pool.c $(FA_PATH)object_pool/node_pool.c $(FA_PATH)object_pool/edge_pool.c $(FA_PATH)hashmap/address_map.c $(FA_PATH)object_pool/graph_pool.c $(FA_PATH)fa.c $(FA_PATH)set/bitset.c $(FA_PATH)array/node_array.c $(FA_PATH)queue/int_queue.c $(FA_PATH)hashmap/intset_map.c $(FA_PATH)hashmap/setint_map.c $(FA_PATH)object_pool/intsetmap_node_pool.c $(FA_PATH)nfa_to_dfa.c $(FA_PATH)array/set_array.c $(FA_PATH)dfa_minimization.c $(FA_PATH)list/set_cross_list.c $(FA_PATH)object_pool/set_cross_list_node_pool.c $(FA_PATH)object_pool/nodelist_node_pool.c $(FA_PATH)object_pool/intlistmap_node_pool.c $(FA_PATH)object_pool/partition_set_pool.c $(FA_PATH)list/node_list.c $(FA_PATH)hashmap/intlist_map.c $(FA_PATH)set/partition.c
	$(CC) -o $@ $^ $(OPTIMIZE) $(CC_GLOBAL_PARAM)


,PHONY clean:
	rm *.exe
