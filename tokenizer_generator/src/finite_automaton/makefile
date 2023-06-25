OPTIMIZE = -g
CC = gcc
AR = ar

AR_OPTION = rcs

ROOT_PATH = $(abspath $(shell pwd)/../../..)/

CC_GLOBAL_PARAM = -Wall -I $(ROOT_PATH)

TOKENIZER_PATH = $(ROOT_PATH)tokenizer_generator/
INC_PATH = $(TOKENIZER_PATH)include/
SRC_PATH = $(TOKENIZER_PATH)src/
BIN_PATH = $(TOKENIZER_PATH)bin/
LIB_PATH = $(TOKENIZER_PATH)lib/

GEN_INC_PATH = $(INC_PATH)general/

#finite_automaton path
FA_INC_PATH = $(INC_PATH)finite_automaton/
FA_PATH = $(SRC_PATH)finite_automaton/
OBJP_INC_PATH = $(FA_INC_PATH)object_pool/
OBJP_PATH = $(FA_PATH)object_pool/
HMAP_INC_PATH = $(FA_INC_PATH)hashmap/
HMAP_PATH = $(FA_PATH)hashmap/
SET_INC_PATH = $(FA_INC_PATH)set/
SET_PATH = $(FA_PATH)set/
ARRAY_INC_PATH = $(FA_INC_PATH)array/
ARRAY_PATH = $(FA_PATH)array/
QUEUE_INC_PATH = $(FA_INC_PATH)queue/
QUEUE_PATH = $(FA_PATH)queue/
LIST_PATH = $(FA_PATH)list/
LIST_INC_PATH = $(FA_INC_PATH)list/



$(BIN_PATH)libfa.a : $(BIN_PATH)graph.o $(BIN_PATH)fa.o $(BIN_PATH)nfa_to_dfa.o $(BIN_PATH)graph_pool.o $(BIN_PATH)node_pool.o $(BIN_PATH)edge_pool.o $(BIN_PATH)intsetmap_node_pool.o $(BIN_PATH)fa_pool.o $(BIN_PATH)address_map.o $(BIN_PATH)intset_map.o $(BIN_PATH)setint_map.o $(BIN_PATH)bitset.o $(BIN_PATH)node_array.o $(BIN_PATH)set_array.o $(BIN_PATH)int_queue.o $(BIN_PATH)set_cross_list.o $(BIN_PATH)set_cross_list_node_pool.o $(BIN_PATH)dfa_minimization.o $(BIN_PATH)partition.o $(BIN_PATH)intlist_map.o $(BIN_PATH)intlistmap_node_pool.o $(BIN_PATH)nodelist_node_pool.o $(BIN_PATH)partition_set_pool.o $(BIN_PATH)node_list.o
	$(AR) $(AR_OPTION) $@ $^

$(BIN_PATH)graph.o : $(FA_PATH)graph.c $(FA_INC_PATH)graph.h $(GEN_INC_PATH)global_def.h $(OBJP_INC_PATH)graph_pool.h $(OBJP_INC_PATH)node_pool.h $(OBJP_INC_PATH)edge_pool.h $(HMAP_INC_PATH)address_map.h
	$(CC) -c -o $@ $< $(OPTIMIZE) $(CC_GLOBAL_PARAM)

$(BIN_PATH)fa.o : $(FA_PATH)finite_automaton.c $(FA_INC_PATH)finite_automaton.h $(GEN_INC_PATH)global_def.h $(FA_INC_PATH)graph.h $(OBJP_INC_PATH)fa_pool.h
	$(CC) -c -o $@ $< $(OPTIMIZE) $(CC_GLOBAL_PARAM)

$(BIN_PATH)nfa_to_dfa.o : $(FA_PATH)nfa_to_dfa.c $(FA_INC_PATH)finite_automaton.h $(FA_INC_PATH)graph.h $(HMAP_INC_PATH)intset_map.h $(HMAP_INC_PATH)setint_map.h $(SET_INC_PATH)bitset.h $(QUEUE_INC_PATH)int_queue.h $(ARRAY_INC_PATH)set_array.h $(ARRAY_INC_PATH)node_array.h $(GEN_INC_PATH)global_def.h
	$(CC) -c -o $@ $< $(OPTIMIZE) $(CC_GLOBAL_PARAM)

$(BIN_PATH)dfa_minimization.o : $(FA_PATH)dfa_minimization.c $(FA_INC_PATH)finite_automaton.h $(FA_INC_PATH)graph.h $(HMAP_INC_PATH)intlist_map.h $(LIST_INC_PATH)set_cross_list.h $(GEN_INC_PATH)global_def.h $(LIST_INC_PATH)node_list.h $(SET_INC_PATH)partition.h
	$(CC) -c -o $@ $< $(OPTIMIZE) $(CC_GLOBAL_PARAM)

$(BIN_PATH)graph_pool.o : $(OBJP_PATH)graph_pool.c  $(OBJP_INC_PATH)graph_pool.h $(FA_INC_PATH)graph.h $(GEN_INC_PATH)global_def.h
	$(CC) -c -o $@ $< $(OPTIMIZE) $(CC_GLOBAL_PARAM)

$(BIN_PATH)node_pool.o : $(OBJP_PATH)node_pool.c  $(OBJP_INC_PATH)node_pool.h $(FA_INC_PATH)graph.h $(GEN_INC_PATH)global_def.h
	$(CC) -c -o $@ $< $(OPTIMIZE) $(CC_GLOBAL_PARAM)

$(BIN_PATH)edge_pool.o : $(OBJP_PATH)edge_pool.c  $(OBJP_INC_PATH)edge_pool.h $(FA_INC_PATH)graph.h $(GEN_INC_PATH)global_def.h
	$(CC) -c -o $@ $< $(OPTIMIZE) $(CC_GLOBAL_PARAM)

$(BIN_PATH)fa_pool.o : $(OBJP_PATH)fa_pool.c  $(OBJP_INC_PATH)fa_pool.h $(FA_INC_PATH)finite_automaton.h $(GEN_INC_PATH)global_def.h
	$(CC) -c -o $@ $< $(OPTIMIZE) $(CC_GLOBAL_PARAM)

$(BIN_PATH)intsetmap_node_pool.o : $(OBJP_PATH)intsetmap_node_pool.c  $(OBJP_INC_PATH)intsetmap_node_pool.h $(SET_INC_PATH)bitset.h $(GEN_INC_PATH)global_def.h
	$(CC) -c -o $@ $< $(OPTIMIZE) $(CC_GLOBAL_PARAM)

$(BIN_PATH)intlistmap_node_pool.o : $(OBJP_PATH)intlistmap_node_pool.c  $(OBJP_INC_PATH)intlistmap_node_pool.h $(FA_INC_PATH)graph.h $(HMAP_INC_PATH)intlist_map.h $(LIST_INC_PATH)node_list.h $(GEN_INC_PATH)global_def.h
	$(CC) -c -o $@ $< $(OPTIMIZE) $(CC_GLOBAL_PARAM)

$(BIN_PATH)set_cross_list_node_pool.o : $(OBJP_PATH)set_cross_list_node_pool.c $(OBJP_INC_PATH)set_cross_list_node_pool.h $(LIST_INC_PATH)set_cross_list.h $(GEN_INC_PATH)global_def.h $(SET_INC_PATH)partition.h
	$(CC) -c -o $@ $< $(OPTIMIZE) $(CC_GLOBAL_PARAM)

$(BIN_PATH)nodelist_node_pool.o : $(OBJP_PATH)nodelist_node_pool.c $(OBJP_INC_PATH)nodelist_node_pool.h $(LIST_INC_PATH)node_list.h $(FA_INC_PATH)graph.h $(GEN_INC_PATH)global_def.h
	$(CC) -c -o $@ $< $(OPTIMIZE) $(CC_GLOBAL_PARAM)

$(BIN_PATH)partition_set_pool.o : $(OBJP_PATH)partition_set_pool.c $(OBJP_INC_PATH)partition_set_pool.h $(SET_INC_PATH)partition.h $(LIST_INC_PATH)node_list.h $(FA_INC_PATH)graph.h $(GEN_INC_PATH)global_def.h
	$(CC) -c -o $@ $< $(OPTIMIZE) $(CC_GLOBAL_PARAM)

$(BIN_PATH)address_map.o : $(HMAP_PATH)address_map.c $(HMAP_INC_PATH)address_map.h $(GEN_INC_PATH)global_def.h
	$(CC) -c -o $@ $< $(OPTIMIZE) $(CC_GLOBAL_PARAM)

$(BIN_PATH)intset_map.o : $(HMAP_PATH)intset_map.c $(HMAP_INC_PATH)intset_map.h $(SET_INC_PATH)bitset.h $(GEN_INC_PATH)global_def.h $(OBJP_INC_PATH)intsetmap_node_pool.h
	$(CC) -c -o $@ $< $(OPTIMIZE) $(CC_GLOBAL_PARAM)

$(BIN_PATH)setint_map.o : $(HMAP_PATH)setint_map.c $(HMAP_INC_PATH)setint_map.h $(SET_INC_PATH)bitset.h $(GEN_INC_PATH)global_def.h
	$(CC) -c -o $@ $< $(OPTIMIZE) $(CC_GLOBAL_PARAM)

$(BIN_PATH)intlist_map.o : $(HMAP_PATH)intlist_map.c $(HMAP_INC_PATH)intlist_map.h $(LIST_INC_PATH)node_list.h $(GEN_INC_PATH)global_def.h $(OBJP_INC_PATH)intlistmap_node_pool.h $(FA_INC_PATH)graph.h
	$(CC) -c -o $@ $< $(OPTIMIZE) $(CC_GLOBAL_PARAM)

$(BIN_PATH)bitset.o : $(SET_PATH)bitset.c $(SET_INC_PATH)bitset.h $(GEN_INC_PATH)global_def.h
	$(CC) -c -o $@ $< $(OPTIMIZE) $(CC_GLOBAL_PARAM)

$(BIN_PATH)partition.o : $(SET_PATH)partition.c $(SET_INC_PATH)partition.h $(LIST_INC_PATH)node_list.h $(FA_INC_PATH)graph.h $(GEN_INC_PATH)global_def.h
	$(CC) -c -o $@ $< $(OPTIMIZE) $(CC_GLOBAL_PARAM)

$(BIN_PATH)node_array.o : $(ARRAY_PATH)node_array.c $(ARRAY_INC_PATH)node_array.h $(FA_INC_PATH)graph.h $(GEN_INC_PATH)global_def.h
	$(CC) -c -o $@ $< $(OPTIMIZE) $(CC_GLOBAL_PARAM)

$(BIN_PATH)set_array.o : $(ARRAY_PATH)set_array.c $(ARRAY_INC_PATH)set_array.h $(SET_INC_PATH)bitset.h $(GEN_INC_PATH)global_def.h
	$(CC) -c -o $@ $< $(OPTIMIZE) $(CC_GLOBAL_PARAM)

$(BIN_PATH)int_queue.o : $(QUEUE_PATH)int_queue.c $(QUEUE_INC_PATH)int_queue.h $(GEN_INC_PATH)global_def.h
	$(CC) -c -o $@ $< $(OPTIMIZE) $(CC_GLOBAL_PARAM)

$(BIN_PATH)set_cross_list.o : $(LIST_PATH)set_cross_list.c $(LIST_INC_PATH)set_cross_list.h $(SET_INC_PATH)bitset.h $(GEN_INC_PATH)global_def.h
	$(CC) -c -o $@ $< $(OPTIMIZE) $(CC_GLOBAL_PARAM)

$(BIN_PATH)node_list.o : $(LIST_PATH)node_list.c $(LIST_INC_PATH)node_list.h $(FA_INC_PATH)graph.h $(GEN_INC_PATH)global_def.h
	$(CC) -c -o $@ $< $(OPTIMIZE) $(CC_GLOBAL_PARAM)



.PHONY: clean

clean :
	rm $(BIN_PATH)*.o $(BIN_PATH)*.a $(BIN_PATH)*.exe
