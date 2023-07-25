INC_PATH = $(UTILS_PATH)include/
SRC_PATH = $(UTILS_PATH)src/
BIN_PATH = $(UTILS_PATH)bin/
LIB_PATH = $(UTILS_PATH)lib/

OBJP_INC_PATH = $(INC_PATH)object_pool/
OBJP_PATH = $(SRC_PATH)object_pool/
HMAP_INC_PATH = $(INC_PATH)hashmap/
HMAP_PATH = $(SRC_PATH)hashmap/
SET_INC_PATH = $(INC_PATH)set/
SET_PATH = $(SRC_PATH)set/
ARRAY_INC_PATH = $(INC_PATH)array/
ARRAY_PATH = $(SRC_PATH)array/
QUEUE_INC_PATH = $(INC_PATH)queue/
QUEUE_PATH = $(SRC_PATH)queue/
LIST_INC_PATH = $(INC_PATH)list/
LIST_PATH = $(SRC_PATH)list/
OS_SPEC_PATH = $(SRC_PATH)os_spec/
OS_SPEC_INC_PATH = $(INC_PATH)os_spec/
GEN_INC_PATH = $(INC_PATH)general/

$(BIN_PATH)utils.a : $(BIN_PATH)dir.o $(BIN_PATH)intsetmap_node_pool.o $(BIN_PATH)intset_map.o $(BIN_PATH)setint_map.o $(BIN_PATH)str_map.o $(BIN_PATH)address_map.o $(BIN_PATH)bitset.o $(BIN_PATH)set_array.o $(BIN_PATH)int_queue.o
	$(AR) $@ $^

$(BIN_PATH)dir.o : $(OS_SPEC_PATH)dir.c $(OS_SPEC_INC_PATH)dir.h
	$(CC) -c -o $@ $< $(CFLAGS) 

$(BIN_PATH)intsetmap_node_pool.o : $(OBJP_PATH)intsetmap_node_pool.c  $(OBJP_INC_PATH)intsetmap_node_pool.h $(SET_INC_PATH)bitset.h $(GEN_INC_PATH)global_def.h
	$(CC) -c -o $@ $< $(CFLAGS)

$(BIN_PATH)address_map.o : $(HMAP_PATH)address_map.c $(HMAP_INC_PATH)address_map.h $(GEN_INC_PATH)global_def.h
	$(CC) -c -o $@ $< $(CFLAGS)

$(BIN_PATH)intset_map.o : $(HMAP_PATH)intset_map.c $(HMAP_INC_PATH)intset_map.h $(SET_INC_PATH)bitset.h $(GEN_INC_PATH)global_def.h $(OBJP_INC_PATH)intsetmap_node_pool.h
	$(CC) -c -o $@ $< $(CFLAGS)

$(BIN_PATH)setint_map.o : $(HMAP_PATH)setint_map.c $(HMAP_INC_PATH)setint_map.h $(SET_INC_PATH)bitset.h $(GEN_INC_PATH)global_def.h
	$(CC) -c -o $@ $< $(CFLAGS)

$(BIN_PATH)str_map.o : $(HMAP_PATH)str_map.c $(HMAP_INC_PATH)str_map.h
	$(CC) -c -o $@ $< $(CFLAGS)

$(BIN_PATH)bitset.o : $(SET_PATH)bitset.c $(SET_INC_PATH)bitset.h $(GEN_INC_PATH)global_def.h
	$(CC) -c -o $@ $< $(CFLAGS)

$(BIN_PATH)set_array.o : $(ARRAY_PATH)set_array.c $(ARRAY_INC_PATH)set_array.h $(SET_INC_PATH)bitset.h $(GEN_INC_PATH)global_def.h
	$(CC) -c -o $@ $< $(CFLAGS)

$(BIN_PATH)int_queue.o : $(QUEUE_PATH)int_queue.c $(QUEUE_INC_PATH)int_queue.h $(GEN_INC_PATH)global_def.h
	$(CC) -c -o $@ $< $(CFLAGS)

.PHONY: clean
clean :
	rm $(BIN_PATH)*
