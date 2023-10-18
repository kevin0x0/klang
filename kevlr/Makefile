INC_DIR = $(KEVLR_DIR)include/
SRC_DIR = $(KEVLR_DIR)src/
LIB_DIR = $(KEVLR_DIR)lib/

OBJP_INC_DIR = $(INC_DIR)objpool/
OBJP_DIR = $(SRC_DIR)objpool/
HMAP_INC_DIR = $(INC_DIR)hashmap/
HMAP_DIR = $(SRC_DIR)hashmap/
SET_INC_DIR = $(INC_DIR)set/
SET_DIR = $(SRC_DIR)set/

UTILS_INC_DIR = $(UTILS_DIR)include/
GEN_INC_DIR = $(UTILS_INC_DIR)general/


$(LIB_DIR)libkevlr.a : $(LIB_DIR)itemset.o $(LIB_DIR)lr_utils.o $(LIB_DIR)lalr.o $(LIB_DIR)lr1.o $(LIB_DIR)slr.o $(LIB_DIR)collection.o $(LIB_DIR)table.o $(LIB_DIR)confhandle.o $(LIB_DIR)print.o $(LIB_DIR)rule.o $(LIB_DIR)itemset_set.o $(LIB_DIR)item_pool.o $(LIB_DIR)itemsettrans_pool.o $(LIB_DIR)itemset_pool.o $(LIB_DIR)transset.o $(LIB_DIR)priority_map.o | create_lib_dir
	cp $(UTILS_DIR)lib/libutils.a $(LIB_DIR)libkevlr.a
	$(AR) $@ $^

$(LIB_DIR)itemset.o : $(SRC_DIR)itemset.c $(INC_DIR)itemset.h $(INC_DIR)itemset_def.h $(INC_DIR)rule.h $(OBJP_INC_DIR)itemsettrans_pool.h $(OBJP_INC_DIR)itemset_pool.h $(OBJP_INC_DIR)item_pool.h $(UTILS_INC_DIR)set/bitset.h $(GEN_INC_DIR)global_def.h | create_lib_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(LIB_DIR)lr_utils.o : $(SRC_DIR)lr_utils.c $(INC_DIR)lr_utils.h $(INC_DIR)collection.h $(INC_DIR)itemset.h $(INC_DIR)itemset_def.h $(INC_DIR)rule.h  $(SET_INC_DIR)transset.h $(GEN_INC_DIR)global_def.h | create_lib_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(LIB_DIR)lalr.o : $(SRC_DIR)lalr.c $(INC_DIR)lr_utils.h $(INC_DIR)collection.h $(UTILS_INC_DIR)set/bitset.h $(INC_DIR)itemset_def.h $(INC_DIR)rule.h $(OBJP_INC_DIR)itemsettrans_pool.h $(OBJP_INC_DIR)itemset_pool.h $(OBJP_INC_DIR)item_pool.h $(SET_INC_DIR)transset.h $(SET_INC_DIR)itemset_set.h $(GEN_INC_DIR)global_def.h | create_lib_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(LIB_DIR)lr1.o : $(SRC_DIR)lr1.c $(INC_DIR)lr_utils.h $(INC_DIR)collection.h $(UTILS_INC_DIR)set/bitset.h $(INC_DIR)itemset_def.h $(INC_DIR)rule.h $(OBJP_INC_DIR)itemsettrans_pool.h $(OBJP_INC_DIR)itemset_pool.h $(OBJP_INC_DIR)item_pool.h $(SET_INC_DIR)transset.h $(SET_INC_DIR)itemset_set.h $(GEN_INC_DIR)global_def.h | create_lib_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(LIB_DIR)slr.o : $(SRC_DIR)slr.c $(INC_DIR)lr_utils.h $(INC_DIR)collection.h $(UTILS_INC_DIR)set/bitset.h $(INC_DIR)itemset_def.h $(INC_DIR)rule.h $(OBJP_INC_DIR)itemsettrans_pool.h $(OBJP_INC_DIR)itemset_pool.h $(OBJP_INC_DIR)item_pool.h $(SET_INC_DIR)transset.h $(SET_INC_DIR)itemset_set.h $(GEN_INC_DIR)global_def.h | create_lib_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(LIB_DIR)collection.o : $(SRC_DIR)collection.c $(INC_DIR)collection.h $(INC_DIR)itemset.h $(INC_DIR)itemset_def.h $(INC_DIR)rule.h $(OBJP_INC_DIR)itemsettrans_pool.h $(OBJP_INC_DIR)itemset_pool.h $(OBJP_INC_DIR)item_pool.h $(UTILS_INC_DIR)set/bitset.h | create_lib_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(LIB_DIR)table.o : $(SRC_DIR)table.c $(INC_DIR)table.h $(INC_DIR)collection.h $(INC_DIR)itemset.h $(INC_DIR)itemset_def.h $(INC_DIR)rule.h $(OBJP_INC_DIR)itemsettrans_pool.h $(OBJP_INC_DIR)itemset_pool.h $(OBJP_INC_DIR)item_pool.h $(UTILS_INC_DIR)set/bitset.h | create_lib_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(LIB_DIR)confhandle.o : $(SRC_DIR)conflict_handle.c $(INC_DIR)conflict_handle.h $(HMAP_INC_DIR)priority_map.h $(INC_DIR)table.h $(INC_DIR)collection.h $(INC_DIR)itemset.h $(INC_DIR)itemset_def.h $(INC_DIR)rule.h | create_lib_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(LIB_DIR)print.o : $(SRC_DIR)print.c $(INC_DIR)print.h $(INC_DIR)itemset.h $(INC_DIR)itemset_def.h $(INC_DIR)rule.h $(OBJP_INC_DIR)itemsettrans_pool.h $(OBJP_INC_DIR)itemset_pool.h $(OBJP_INC_DIR)item_pool.h $(UTILS_INC_DIR)set/bitset.h $(GEN_INC_DIR)global_def.h  | create_lib_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(LIB_DIR)rule.o : $(SRC_DIR)rule.c $(INC_DIR)rule.h $(UTILS_INC_DIR)set/hashset.h $(UTILS_INC_DIR)queue/addr_queue.h $(UTILS_INC_DIR)string/kev_string.h $(GEN_INC_DIR)global_def.h | create_lib_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(LIB_DIR)itemset_set.o : $(SET_DIR)itemset_set.c $(SET_INC_DIR)itemset_set.h $(INC_DIR)itemset_def.h $(INC_DIR)rule.h $(GEN_INC_DIR)global_def.h | create_lib_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(LIB_DIR)transset.o : $(SET_DIR)transset.c $(SET_INC_DIR)transset.h $(INC_DIR)lr.h $(UTILS_INC_DIR)set/bitset.h $(INC_DIR)itemset_def.h $(INC_DIR)rule.h $(OBJP_INC_DIR)itemsettrans_pool.h $(OBJP_INC_DIR)itemset_pool.h $(OBJP_INC_DIR)item_pool.h $(GEN_INC_DIR)global_def.h | create_lib_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(LIB_DIR)priority_map.o : $(HMAP_DIR)priority_map.c $(HMAP_INC_DIR)priority_map.h $(INC_DIR)rule.h $(GEN_INC_DIR)global_def.h | create_lib_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(LIB_DIR)itemset_pool.o : $(OBJP_DIR)itemset_pool.c $(OBJP_INC_DIR)itemset_pool.h $(INC_DIR)itemset_def.h $(INC_DIR)rule.h $(UTILS_INC_DIR)set/bitset.h $(GEN_INC_DIR)global_def.h | create_lib_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(LIB_DIR)itemsettrans_pool.o : $(OBJP_DIR)itemsettrans_pool.c $(OBJP_INC_DIR)itemsettrans_pool.h $(INC_DIR)itemset_def.h $(INC_DIR)rule.h $(UTILS_INC_DIR)set/bitset.h | create_lib_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(LIB_DIR)item_pool.o : $(OBJP_DIR)item_pool.c $(OBJP_INC_DIR)item_pool.h $(INC_DIR)itemset_def.h $(INC_DIR)rule.h $(UTILS_INC_DIR)set/bitset.h | create_lib_dir
	$(CC) -c -o $@ $< $(CFLAGS)


.PHONY: clean create_lib_dir
clean :
	$(RM) $(LIB_DIR)*

create_lib_dir :
	mkdir -p $(LIB_DIR)
