INC_DIR = $(PARGEN_DIR)include/
SRC_DIR = $(PARGEN_DIR)src/
BIN_DIR = $(PARGEN_DIR)bin/
LIB_DIR = $(PARGEN_DIR)lib/

#finite_automaton path
LR_INC_DIR = $(INC_DIR)lr/
LR_DIR = $(SRC_DIR)lr/
OBJP_INC_DIR = $(LR_INC_DIR)object_pool/
OBJP_DIR = $(LR_DIR)object_pool/
HMAP_INC_DIR = $(LR_INC_DIR)hashmap/
HMAP_DIR = $(LR_DIR)hashmap/
SET_INC_DIR = $(LR_INC_DIR)set/
SET_DIR = $(LR_DIR)set/

UTILS_INC_DIR = $(UTILS_DIR)include/
GEN_INC_DIR = $(UTILS_INC_DIR)general/

$(BIN_DIR)liblr.a : $(BIN_DIR)item.o $(BIN_DIR)lr_utils.o $(BIN_DIR)lalr.o $(BIN_DIR)lr1.o $(BIN_DIR)collection.o $(BIN_DIR)table.o $(BIN_DIR)print.o $(BIN_DIR)rule.o $(BIN_DIR)itemset_set.o $(BIN_DIR)kernel_item_pool.o $(BIN_DIR)itemsetgoto_pool.o $(BIN_DIR)itemset_pool.o $(BIN_DIR)gotomap_node_pool.o $(BIN_DIR)gotomap.o
	$(AR) $@ $^

$(BIN_DIR)item.o : $(LR_DIR)item.c $(LR_INC_DIR)item.h $(LR_INC_DIR)item_def.h $(LR_INC_DIR)rule.h $(OBJP_INC_DIR)itemsetgoto_pool.h $(OBJP_INC_DIR)itemset_pool.h $(OBJP_INC_DIR)kernel_item_pool.h $(UTILS_INC_DIR)set/bitset.h $(GEN_INC_DIR)global_def.h
	$(CC) -c -o $@ $< $(CFLAGS)

$(BIN_DIR)lr_utils.o : $(LR_DIR)lr_utils.c $(LR_INC_DIR)lr_utils.h $(LR_INC_DIR)collection.h $(LR_INC_DIR)item.h $(LR_INC_DIR)item_def.h $(LR_INC_DIR)rule.h  $(HMAP_INC_DIR)gotomap.h $(UTILS_INC_DIR)array/addr_array.h $(GEN_INC_DIR)global_def.h 
	$(CC) -c -o $@ $< $(CFLAGS)

$(BIN_DIR)lalr.o : $(LR_DIR)lalr.c $(LR_INC_DIR)lalr.h $(LR_INC_DIR)lr_utils.h $(LR_INC_DIR)collection.h $(UTILS_INC_DIR)set/bitset.h $(LR_INC_DIR)item_def.h $(LR_INC_DIR)rule.h $(OBJP_INC_DIR)itemsetgoto_pool.h $(OBJP_INC_DIR)itemset_pool.h $(OBJP_INC_DIR)kernel_item_pool.h $(HMAP_INC_DIR)gotomap.h $(SET_INC_DIR)itemset_set.h $(UTILS_INC_DIR)array/addr_array.h $(GEN_INC_DIR)global_def.h 
	$(CC) -c -o $@ $< $(CFLAGS)

$(BIN_DIR)lr1.o : $(LR_DIR)lr1.c $(LR_INC_DIR)lr1.h $(LR_INC_DIR)lr_utils.h $(LR_INC_DIR)collection.h $(UTILS_INC_DIR)set/bitset.h $(LR_INC_DIR)item_def.h $(LR_INC_DIR)rule.h $(OBJP_INC_DIR)itemsetgoto_pool.h $(OBJP_INC_DIR)itemset_pool.h $(OBJP_INC_DIR)kernel_item_pool.h $(HMAP_INC_DIR)gotomap.h $(SET_INC_DIR)itemset_set.h $(UTILS_INC_DIR)array/addr_array.h $(GEN_INC_DIR)global_def.h 
	$(CC) -c -o $@ $< $(CFLAGS)

$(BIN_DIR)collection.o : $(LR_DIR)collection.c $(LR_INC_DIR)collection.h $(LR_INC_DIR)item.h $(LR_INC_DIR)item_def.h $(LR_INC_DIR)rule.h $(OBJP_INC_DIR)itemsetgoto_pool.h $(OBJP_INC_DIR)itemset_pool.h $(OBJP_INC_DIR)kernel_item_pool.h $(UTILS_INC_DIR)set/bitset.h
	$(CC) -c -o $@ $< $(CFLAGS)

$(BIN_DIR)table.o : $(LR_DIR)table.c $(LR_INC_DIR)table.h $(LR_INC_DIR)collection.h $(LR_INC_DIR)item.h $(LR_INC_DIR)item_def.h $(LR_INC_DIR)rule.h $(OBJP_INC_DIR)itemsetgoto_pool.h $(OBJP_INC_DIR)itemset_pool.h $(OBJP_INC_DIR)kernel_item_pool.h $(UTILS_INC_DIR)set/bitset.h
	$(CC) -c -o $@ $< $(CFLAGS)

$(BIN_DIR)print.o : $(LR_DIR)print.c $(LR_INC_DIR)print.h $(LR_INC_DIR)item.h $(LR_INC_DIR)item_def.h $(LR_INC_DIR)rule.h $(OBJP_INC_DIR)itemsetgoto_pool.h $(OBJP_INC_DIR)itemset_pool.h $(OBJP_INC_DIR)kernel_item_pool.h $(UTILS_INC_DIR)set/bitset.h $(UTILS_INC_DIR)array/addr_array.h $(GEN_INC_DIR)global_def.h 
	$(CC) -c -o $@ $< $(CFLAGS)

$(BIN_DIR)rule.o : $(LR_DIR)rule.c $(LR_INC_DIR)rule.h $(UTILS_INC_DIR)set/hashset.h $(UTILS_INC_DIR)array/addr_array.h $(UTILS_INC_DIR)queue/addr_queue.h $(GEN_INC_DIR)global_def.h
	$(CC) -c -o $@ $< $(CFLAGS)

$(BIN_DIR)itemset_set.o : $(SET_DIR)itemset_set.c $(SET_INC_DIR)itemset_set.h $(LR_INC_DIR)item.h $(LR_INC_DIR)item_def.h $(LR_INC_DIR)rule.h $(OBJP_INC_DIR)itemsetgoto_pool.h $(OBJP_INC_DIR)itemset_pool.h $(OBJP_INC_DIR)kernel_item_pool.h $(GEN_INC_DIR)global_def.h
	$(CC) -c -o $@ $< $(CFLAGS)

$(BIN_DIR)gotomap.o : $(HMAP_DIR)gotomap.c $(HMAP_INC_DIR)gotomap.h $(LR_INC_DIR)lr.h $(LR_INC_DIR)lalr.h $(UTILS_INC_DIR)set/bitset.h $(LR_INC_DIR)item_def.h $(LR_INC_DIR)rule.h $(OBJP_INC_DIR)itemsetgoto_pool.h $(OBJP_INC_DIR)itemset_pool.h $(OBJP_INC_DIR)kernel_item_pool.h $(GEN_INC_DIR)global_def.h
	$(CC) -c -o $@ $< $(CFLAGS)

$(BIN_DIR)gotomap_node_pool.o : $(OBJP_DIR)gotomap_node_pool.c $(OBJP_INC_DIR)gotomap_node_pool.h $(HMAP_INC_DIR)gotomap.h $(LR_INC_DIR)lr.h $(LR_INC_DIR)lalr.h $(UTILS_INC_DIR)set/bitset.h $(LR_INC_DIR)item_def.h $(LR_INC_DIR)rule.h $(OBJP_INC_DIR)itemsetgoto_pool.h $(OBJP_INC_DIR)itemset_pool.h $(OBJP_INC_DIR)kernel_item_pool.h $(GEN_INC_DIR)global_def.h
	$(CC) -c -o $@ $< $(CFLAGS)

$(BIN_DIR)itemset_pool.o : $(OBJP_DIR)itemset_pool.c $(OBJP_INC_DIR)itemset_pool.h $(LR_INC_DIR)item_def.h $(LR_INC_DIR)rule.h $(UTILS_INC_DIR)set/bitset.h $(GEN_INC_DIR)global_def.h
	$(CC) -c -o $@ $< $(CFLAGS)

$(BIN_DIR)itemsetgoto_pool.o : $(OBJP_DIR)itemsetgoto_pool.c $(OBJP_INC_DIR)itemsetgoto_pool.h $(LR_INC_DIR)item_def.h $(LR_INC_DIR)rule.h $(UTILS_INC_DIR)set/bitset.h
	$(CC) -c -o $@ $< $(CFLAGS)

$(BIN_DIR)kernel_item_pool.o : $(OBJP_DIR)kernel_item_pool.c $(OBJP_INC_DIR)kernel_item_pool.h $(LR_INC_DIR)item_def.h $(LR_INC_DIR)rule.h $(UTILS_INC_DIR)set/bitset.h
	$(CC) -c -o $@ $< $(CFLAGS)
