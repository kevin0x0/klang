INC_DIR = $(UTILS_DIR)include/
SRC_DIR = $(UTILS_DIR)src/
BIN_DIR = $(UTILS_DIR)bin/
LIB_DIR = $(UTILS_DIR)lib/

OBJP_INC_DIR = $(INC_DIR)object_pool/
OBJP_DIR = $(SRC_DIR)object_pool/
HMAP_INC_DIR = $(INC_DIR)hashmap/
HMAP_DIR = $(SRC_DIR)hashmap/
SET_INC_DIR = $(INC_DIR)set/
SET_DIR = $(SRC_DIR)set/
ARRAY_INC_DIR = $(INC_DIR)array/
ARRAY_DIR = $(SRC_DIR)array/
QUEUE_INC_DIR = $(INC_DIR)queue/
QUEUE_DIR = $(SRC_DIR)queue/
LIST_INC_DIR = $(INC_DIR)list/
LIST_DIR = $(SRC_DIR)list/
OS_SPEC_DIR = $(SRC_DIR)os_spec/
OS_SPEC_INC_DIR = $(INC_DIR)os_spec/
GEN_INC_DIR = $(INC_DIR)general/

all : $(BIN_DIR) $(BIN_DIR)libutils.a


$(BIN_DIR)libutils.a : $(BIN_DIR)dir.o $(BIN_DIR)intsetmap_node_pool.o $(BIN_DIR)intset_map.o $(BIN_DIR)setint_map.o $(BIN_DIR)str_map.o $(BIN_DIR)address_map.o $(BIN_DIR)bitset.o $(BIN_DIR)set_array.o $(BIN_DIR)int_queue.o
	$(AR) $@ $^

$(BIN_DIR) :
	mkdir $(BIN_DIR)

$(BIN_DIR)dir.o : $(OS_SPEC_DIR)dir.c $(OS_SPEC_INC_DIR)dir.h
	$(CC) -c -o $@ $< $(CFLAGS) 

$(BIN_DIR)intsetmap_node_pool.o : $(OBJP_DIR)intsetmap_node_pool.c  $(OBJP_INC_DIR)intsetmap_node_pool.h $(SET_INC_DIR)bitset.h $(GEN_INC_DIR)global_def.h
	$(CC) -c -o $@ $< $(CFLAGS)

$(BIN_DIR)address_map.o : $(HMAP_DIR)address_map.c $(HMAP_INC_DIR)address_map.h $(GEN_INC_DIR)global_def.h
	$(CC) -c -o $@ $< $(CFLAGS)

$(BIN_DIR)intset_map.o : $(HMAP_DIR)intset_map.c $(HMAP_INC_DIR)intset_map.h $(SET_INC_DIR)bitset.h $(GEN_INC_DIR)global_def.h $(OBJP_INC_DIR)intsetmap_node_pool.h
	$(CC) -c -o $@ $< $(CFLAGS)

$(BIN_DIR)setint_map.o : $(HMAP_DIR)setint_map.c $(HMAP_INC_DIR)setint_map.h $(SET_INC_DIR)bitset.h $(GEN_INC_DIR)global_def.h
	$(CC) -c -o $@ $< $(CFLAGS)

$(BIN_DIR)str_map.o : $(HMAP_DIR)str_map.c $(HMAP_INC_DIR)str_map.h
	$(CC) -c -o $@ $< $(CFLAGS)

$(BIN_DIR)bitset.o : $(SET_DIR)bitset.c $(SET_INC_DIR)bitset.h $(GEN_INC_DIR)global_def.h
	$(CC) -c -o $@ $< $(CFLAGS)

$(BIN_DIR)set_array.o : $(ARRAY_DIR)set_array.c $(ARRAY_INC_DIR)set_array.h $(SET_INC_DIR)bitset.h $(GEN_INC_DIR)global_def.h
	$(CC) -c -o $@ $< $(CFLAGS)

$(BIN_DIR)int_queue.o : $(QUEUE_DIR)int_queue.c $(QUEUE_INC_DIR)int_queue.h $(GEN_INC_DIR)global_def.h
	$(CC) -c -o $@ $< $(CFLAGS)

.PHONY: clean
clean :
	rm $(BIN_DIR)*
