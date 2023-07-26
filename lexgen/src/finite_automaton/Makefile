INC_DIR = $(LEXGEN_DIR)include/
SRC_DIR = $(LEXGEN_DIR)src/
BIN_DIR = $(LEXGEN_DIR)bin/
LIB_DIR = $(LEXGEN_DIR)lib/

#finite_automaton path
FA_INC_DIR = $(INC_DIR)finite_automaton/
FA_DIR = $(SRC_DIR)finite_automaton/
OBJP_INC_DIR = $(FA_INC_DIR)object_pool/
OBJP_DIR = $(FA_DIR)object_pool/
HMAP_INC_DIR = $(FA_INC_DIR)hashmap/
HMAP_DIR = $(FA_DIR)hashmap/
SET_INC_DIR = $(FA_INC_DIR)set/
SET_DIR = $(FA_DIR)set/
ARRAY_INC_DIR = $(FA_INC_DIR)array/
ARRAY_DIR = $(FA_DIR)array/
LIST_INC_DIR = $(FA_INC_DIR)list/
LIST_DIR = $(FA_DIR)list/

UTILS_INC_DIR = $(UTILS_DIR)include/
GEN_INC_DIR = $(UTILS_INC_DIR)general/


$(BIN_DIR)libfa.a : $(BIN_DIR)graph.o $(BIN_DIR)fa.o $(BIN_DIR)nfa_to_dfa.o $(BIN_DIR)graph_pool.o $(BIN_DIR)node_pool.o $(BIN_DIR)edge_pool.o $(BIN_DIR)fa_pool.o $(BIN_DIR)node_array.o $(BIN_DIR)set_cross_list.o $(BIN_DIR)set_cross_list_node_pool.o $(BIN_DIR)dfa_minimization.o $(BIN_DIR)partition.o $(BIN_DIR)intlist_map.o $(BIN_DIR)intlistmap_node_pool.o $(BIN_DIR)nodelist_node_pool.o $(BIN_DIR)partition_set_pool.o $(BIN_DIR)node_list.o
	$(AR) $@ $^

$(BIN_DIR)graph.o : $(FA_DIR)graph.c $(FA_INC_DIR)graph.h $(GEN_INC_DIR)global_def.h $(OBJP_INC_DIR)graph_pool.h $(OBJP_INC_DIR)node_pool.h $(OBJP_INC_DIR)edge_pool.h $(UTILS_INC_DIR)hashmap/address_map.h
	$(CC) -c -o $@ $< $(CFLAGS)

$(BIN_DIR)fa.o : $(FA_DIR)finite_automaton.c $(FA_INC_DIR)finite_automaton.h $(GEN_INC_DIR)global_def.h $(FA_INC_DIR)graph.h $(OBJP_INC_DIR)fa_pool.h
	$(CC) -c -o $@ $< $(CFLAGS)

$(BIN_DIR)nfa_to_dfa.o : $(FA_DIR)nfa_to_dfa.c $(FA_INC_DIR)finite_automaton.h $(FA_INC_DIR)graph.h $(UTILS_INC_DIR)hashmap/intset_map.h $(UTILS_INC_DIR)hashmap/setint_map.h $(UTILS_INC_DIR)set/bitset.h $(UTILS_INC_DIR)queue/int_queue.h $(UTILS_INC_DIR)array/set_array.h $(ARRAY_INC_DIR)node_array.h $(GEN_INC_DIR)global_def.h
	$(CC) -c -o $@ $< $(CFLAGS)

$(BIN_DIR)dfa_minimization.o : $(FA_DIR)dfa_minimization.c $(FA_INC_DIR)finite_automaton.h $(FA_INC_DIR)graph.h $(HMAP_INC_DIR)intlist_map.h $(LIST_INC_DIR)set_cross_list.h $(GEN_INC_DIR)global_def.h $(LIST_INC_DIR)node_list.h $(SET_INC_DIR)partition.h
	$(CC) -c -o $@ $< $(CFLAGS)

$(BIN_DIR)graph_pool.o : $(OBJP_DIR)graph_pool.c  $(OBJP_INC_DIR)graph_pool.h $(FA_INC_DIR)graph.h $(GEN_INC_DIR)global_def.h
	$(CC) -c -o $@ $< $(CFLAGS)

$(BIN_DIR)node_pool.o : $(OBJP_DIR)node_pool.c  $(OBJP_INC_DIR)node_pool.h $(FA_INC_DIR)graph.h $(GEN_INC_DIR)global_def.h
	$(CC) -c -o $@ $< $(CFLAGS)

$(BIN_DIR)edge_pool.o : $(OBJP_DIR)edge_pool.c  $(OBJP_INC_DIR)edge_pool.h $(FA_INC_DIR)graph.h $(GEN_INC_DIR)global_def.h
	$(CC) -c -o $@ $< $(CFLAGS)

$(BIN_DIR)fa_pool.o : $(OBJP_DIR)fa_pool.c  $(OBJP_INC_DIR)fa_pool.h $(FA_INC_DIR)finite_automaton.h $(GEN_INC_DIR)global_def.h
	$(CC) -c -o $@ $< $(CFLAGS)

$(BIN_DIR)intlistmap_node_pool.o : $(OBJP_DIR)intlistmap_node_pool.c  $(OBJP_INC_DIR)intlistmap_node_pool.h $(FA_INC_DIR)graph.h $(HMAP_INC_DIR)intlist_map.h $(LIST_INC_DIR)node_list.h $(GEN_INC_DIR)global_def.h
	$(CC) -c -o $@ $< $(CFLAGS)

$(BIN_DIR)set_cross_list_node_pool.o : $(OBJP_DIR)set_cross_list_node_pool.c $(OBJP_INC_DIR)set_cross_list_node_pool.h $(LIST_INC_DIR)set_cross_list.h $(GEN_INC_DIR)global_def.h $(SET_INC_DIR)partition.h
	$(CC) -c -o $@ $< $(CFLAGS)

$(BIN_DIR)nodelist_node_pool.o : $(OBJP_DIR)nodelist_node_pool.c $(OBJP_INC_DIR)nodelist_node_pool.h $(LIST_INC_DIR)node_list.h $(FA_INC_DIR)graph.h $(GEN_INC_DIR)global_def.h
	$(CC) -c -o $@ $< $(CFLAGS)

$(BIN_DIR)partition_set_pool.o : $(OBJP_DIR)partition_set_pool.c $(OBJP_INC_DIR)partition_set_pool.h $(SET_INC_DIR)partition.h $(LIST_INC_DIR)node_list.h $(FA_INC_DIR)graph.h $(GEN_INC_DIR)global_def.h
	$(CC) -c -o $@ $< $(CFLAGS)

$(BIN_DIR)intlist_map.o : $(HMAP_DIR)intlist_map.c $(HMAP_INC_DIR)intlist_map.h $(LIST_INC_DIR)node_list.h $(GEN_INC_DIR)global_def.h $(OBJP_INC_DIR)intlistmap_node_pool.h $(FA_INC_DIR)graph.h
	$(CC) -c -o $@ $< $(CFLAGS)

$(BIN_DIR)partition.o : $(SET_DIR)partition.c $(SET_INC_DIR)partition.h $(LIST_INC_DIR)node_list.h $(FA_INC_DIR)graph.h $(GEN_INC_DIR)global_def.h
	$(CC) -c -o $@ $< $(CFLAGS)

$(BIN_DIR)node_array.o : $(ARRAY_DIR)node_array.c $(ARRAY_INC_DIR)node_array.h $(FA_INC_DIR)graph.h $(GEN_INC_DIR)global_def.h
	$(CC) -c -o $@ $< $(CFLAGS)

$(BIN_DIR)set_cross_list.o : $(LIST_DIR)set_cross_list.c $(LIST_INC_DIR)set_cross_list.h $(UTILS_INC_DIR)set/bitset.h $(GEN_INC_DIR)global_def.h
	$(CC) -c -o $@ $< $(CFLAGS)

$(BIN_DIR)node_list.o : $(LIST_DIR)node_list.c $(LIST_INC_DIR)node_list.h $(FA_INC_DIR)graph.h $(GEN_INC_DIR)global_def.h
	$(CC) -c -o $@ $< $(CFLAGS)
