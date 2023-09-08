INC_DIR = $(KEVFA_DIR)include/
SRC_DIR = $(KEVFA_DIR)src/
LIB_DIR = $(KEVFA_DIR)lib/

OBJP_INC_DIR = $(INC_DIR)object_pool/
OBJP_DIR = $(SRC_DIR)object_pool/
HMAP_INC_DIR = $(INC_DIR)hashmap/
HMAP_DIR = $(SRC_DIR)hashmap/
SET_INC_DIR = $(INC_DIR)set/
SET_DIR = $(SRC_DIR)set/
ARRAY_INC_DIR = $(INC_DIR)array/
ARRAY_DIR = $(SRC_DIR)array/
LIST_INC_DIR = $(INC_DIR)list/
LIST_DIR = $(SRC_DIR)list/

UTILS_INC_DIR = $(UTILS_DIR)include/
GEN_INC_DIR = $(UTILS_INC_DIR)general/



$(LIB_DIR)libkevfa.a : $(LIB_DIR)graph.o $(LIB_DIR)fa.o $(LIB_DIR)nfa_to_dfa.o $(LIB_DIR)graph_pool.o $(LIB_DIR)node_pool.o $(LIB_DIR)edge_pool.o $(LIB_DIR)fa_pool.o $(LIB_DIR)node_array.o $(LIB_DIR)set_cross_list.o $(LIB_DIR)set_cross_list_node_pool.o $(LIB_DIR)dfa_minimization.o $(LIB_DIR)partition.o $(LIB_DIR)intlist_map.o $(LIB_DIR)intlistmap_node_pool.o $(LIB_DIR)nodelist_node_pool.o $(LIB_DIR)partition_set_pool.o $(LIB_DIR)node_list.o | create_lib_dir
	cp $(UTILS_DIR)lib/libutils.a $(LIB_DIR)libkevfa.a
	$(AR) $@ $^

$(LIB_DIR)graph.o : $(SRC_DIR)graph.c $(INC_DIR)graph.h $(GEN_INC_DIR)global_def.h $(OBJP_INC_DIR)graph_pool.h $(OBJP_INC_DIR)node_pool.h $(OBJP_INC_DIR)edge_pool.h $(UTILS_INC_DIR)hashmap/address_map.h | create_lib_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(LIB_DIR)fa.o : $(SRC_DIR)finite_automaton.c $(INC_DIR)finite_automaton.h $(GEN_INC_DIR)global_def.h $(INC_DIR)graph.h $(OBJP_INC_DIR)fa_pool.h | create_lib_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(LIB_DIR)nfa_to_dfa.o : $(SRC_DIR)nfa_to_dfa.c $(INC_DIR)finite_automaton.h $(INC_DIR)graph.h $(UTILS_INC_DIR)hashmap/intset_map.h $(UTILS_INC_DIR)hashmap/setint_map.h $(UTILS_INC_DIR)set/bitset.h $(UTILS_INC_DIR)queue/int_queue.h $(UTILS_INC_DIR)array/set_array.h $(ARRAY_INC_DIR)node_array.h $(GEN_INC_DIR)global_def.h | create_lib_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(LIB_DIR)dfa_minimization.o : $(SRC_DIR)dfa_minimization.c $(INC_DIR)finite_automaton.h $(INC_DIR)graph.h $(HMAP_INC_DIR)intlist_map.h $(LIST_INC_DIR)set_cross_list.h $(GEN_INC_DIR)global_def.h $(LIST_INC_DIR)node_list.h $(SET_INC_DIR)partition.h | create_lib_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(LIB_DIR)graph_pool.o : $(OBJP_DIR)graph_pool.c  $(OBJP_INC_DIR)graph_pool.h $(INC_DIR)graph.h $(GEN_INC_DIR)global_def.h | create_lib_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(LIB_DIR)node_pool.o : $(OBJP_DIR)node_pool.c  $(OBJP_INC_DIR)node_pool.h $(INC_DIR)graph.h $(GEN_INC_DIR)global_def.h | create_lib_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(LIB_DIR)edge_pool.o : $(OBJP_DIR)edge_pool.c  $(OBJP_INC_DIR)edge_pool.h $(INC_DIR)graph.h $(GEN_INC_DIR)global_def.h | create_lib_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(LIB_DIR)fa_pool.o : $(OBJP_DIR)fa_pool.c  $(OBJP_INC_DIR)fa_pool.h $(INC_DIR)finite_automaton.h $(GEN_INC_DIR)global_def.h | create_lib_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(LIB_DIR)intlistmap_node_pool.o : $(OBJP_DIR)intlistmap_node_pool.c  $(OBJP_INC_DIR)intlistmap_node_pool.h $(INC_DIR)graph.h $(HMAP_INC_DIR)intlist_map.h $(LIST_INC_DIR)node_list.h $(GEN_INC_DIR)global_def.h | create_lib_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(LIB_DIR)set_cross_list_node_pool.o : $(OBJP_DIR)set_cross_list_node_pool.c $(OBJP_INC_DIR)set_cross_list_node_pool.h $(LIST_INC_DIR)set_cross_list.h $(GEN_INC_DIR)global_def.h $(SET_INC_DIR)partition.h | create_lib_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(LIB_DIR)nodelist_node_pool.o : $(OBJP_DIR)nodelist_node_pool.c $(OBJP_INC_DIR)nodelist_node_pool.h $(LIST_INC_DIR)node_list.h $(INC_DIR)graph.h $(GEN_INC_DIR)global_def.h | create_lib_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(LIB_DIR)partition_set_pool.o : $(OBJP_DIR)partition_set_pool.c $(OBJP_INC_DIR)partition_set_pool.h $(SET_INC_DIR)partition.h $(LIST_INC_DIR)node_list.h $(INC_DIR)graph.h $(GEN_INC_DIR)global_def.h | create_lib_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(LIB_DIR)intlist_map.o : $(HMAP_DIR)intlist_map.c $(HMAP_INC_DIR)intlist_map.h $(LIST_INC_DIR)node_list.h $(GEN_INC_DIR)global_def.h $(OBJP_INC_DIR)intlistmap_node_pool.h $(INC_DIR)graph.h | create_lib_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(LIB_DIR)partition.o : $(SET_DIR)partition.c $(SET_INC_DIR)partition.h $(LIST_INC_DIR)node_list.h $(INC_DIR)graph.h $(GEN_INC_DIR)global_def.h | create_lib_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(LIB_DIR)node_array.o : $(ARRAY_DIR)node_array.c $(ARRAY_INC_DIR)node_array.h $(INC_DIR)graph.h $(GEN_INC_DIR)global_def.h | create_lib_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(LIB_DIR)set_cross_list.o : $(LIST_DIR)set_cross_list.c $(LIST_INC_DIR)set_cross_list.h $(UTILS_INC_DIR)set/bitset.h $(GEN_INC_DIR)global_def.h | create_lib_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(LIB_DIR)node_list.o : $(LIST_DIR)node_list.c $(LIST_INC_DIR)node_list.h $(INC_DIR)graph.h $(GEN_INC_DIR)global_def.h | create_lib_dir
	$(CC) -c -o $@ $< $(CFLAGS)


.PHONY: clean craete_lib_dir
clean :
	$(RM) $(LIB_DIR)*

create_lib_dir :
	mkdir -p $(LIB_DIR)
