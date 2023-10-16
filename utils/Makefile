INC_DIR = $(UTILS_DIR)include/
SRC_DIR = $(UTILS_DIR)src/
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
STR_DIR = $(SRC_DIR)string/
STR_INC_DIR = $(INC_DIR)string/
KSTR_DIR = $(SRC_DIR)kstring/
KSTR_INC_DIR = $(INC_DIR)kstring/
KIO_DIR = $(SRC_DIR)kio/
KIO_INC_DIR = $(INC_DIR)kio/

GEN_INC_DIR = $(INC_DIR)general/



$(LIB_DIR)libutils.a : $(LIB_DIR)dir.o $(LIB_DIR)intsetmap_node_pool.o $(LIB_DIR)intset_map.o $(LIB_DIR)setint_map.o $(LIB_DIR)str_map.o $(LIB_DIR)strx_map.o $(LIB_DIR)func_map.o $(LIB_DIR)address_map.o $(LIB_DIR)bitset.o $(LIB_DIR)hashset.o $(LIB_DIR)set_array.o $(LIB_DIR)int_queue.o $(LIB_DIR)addr_queue.o $(LIB_DIR)karray.o $(LIB_DIR)kev_string.o $(LIB_DIR)kstring.o $(LIB_DIR)kio.o | create_lib_dir
	$(AR) $@ $^


$(LIB_DIR)dir.o : $(OS_SPEC_DIR)dir.c $(OS_SPEC_INC_DIR)dir.h | create_lib_dir
	$(CC) -c -o $@ $< $(CFLAGS) 

$(LIB_DIR)kev_string.o : $(STR_DIR)kev_string.c $(STR_INC_DIR)kev_string.h | create_lib_dir
	$(CC) -c -o $@ $< $(CFLAGS) 

$(LIB_DIR)kstring.o : $(KSTR_DIR)kstring.c $(KSTR_INC_DIR)kstring.h | create_lib_dir
	$(CC) -c -o $@ $< $(CFLAGS) 

$(LIB_DIR)intsetmap_node_pool.o : $(OBJP_DIR)intsetmap_node_pool.c  $(OBJP_INC_DIR)intsetmap_node_pool.h $(SET_INC_DIR)bitset.h $(GEN_INC_DIR)global_def.h | create_lib_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(LIB_DIR)address_map.o : $(HMAP_DIR)address_map.c $(HMAP_INC_DIR)address_map.h $(GEN_INC_DIR)global_def.h | create_lib_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(LIB_DIR)intset_map.o : $(HMAP_DIR)intset_map.c $(HMAP_INC_DIR)intset_map.h $(SET_INC_DIR)bitset.h $(GEN_INC_DIR)global_def.h $(OBJP_INC_DIR)intsetmap_node_pool.h | create_lib_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(LIB_DIR)setint_map.o : $(HMAP_DIR)setint_map.c $(HMAP_INC_DIR)setint_map.h $(SET_INC_DIR)bitset.h $(GEN_INC_DIR)global_def.h | create_lib_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(LIB_DIR)str_map.o : $(HMAP_DIR)str_map.c $(HMAP_INC_DIR)str_map.h | create_lib_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(LIB_DIR)strx_map.o : $(HMAP_DIR)strx_map.c $(HMAP_INC_DIR)strx_map.h $(STR_INC_DIR)kev_string.h $(GEN_INC_DIR)global_def.h | create_lib_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(LIB_DIR)func_map.o : $(HMAP_DIR)func_map.c $(HMAP_INC_DIR)func_map.h $(HMAP_INC_DIR)strx_map.h $(STR_INC_DIR)kev_string.h $(GEN_INC_DIR)global_def.h | create_lib_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(LIB_DIR)bitset.o : $(SET_DIR)bitset.c $(SET_INC_DIR)bitset.h $(GEN_INC_DIR)global_def.h | create_lib_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(LIB_DIR)hashset.o : $(SET_DIR)hashset.c $(SET_INC_DIR)hashset.h $(GEN_INC_DIR)global_def.h | create_lib_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(LIB_DIR)set_array.o : $(ARRAY_DIR)set_array.c $(ARRAY_INC_DIR)set_array.h $(SET_INC_DIR)bitset.h $(GEN_INC_DIR)global_def.h | create_lib_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(LIB_DIR)karray.o : $(ARRAY_DIR)karray.c $(ARRAY_INC_DIR)karray.h $(GEN_INC_DIR)global_def.h | create_lib_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(LIB_DIR)int_queue.o : $(QUEUE_DIR)int_queue.c $(QUEUE_INC_DIR)int_queue.h $(GEN_INC_DIR)global_def.h | create_lib_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(LIB_DIR)addr_queue.o : $(QUEUE_DIR)addr_queue.c $(QUEUE_INC_DIR)addr_queue.h $(GEN_INC_DIR)global_def.h | create_lib_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(LIB_DIR)plugin.o : $(OS_SPEC_DIR)plugin.c $(OS_SPEC_INC_DIR)plugin.h | create_lib_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(LIB_DIR)kio.o : $(KIO_DIR)kio.c $(KIO_INC_DIR)kio.h | create_lib_dir
	$(CC) -c -o $@ $< $(CFLAGS)


.PHONY: clean create_lib_dir
clean :
	$(RM) $(LIB_DIR)*

create_lib_dir :
	mkdir -p $(LIB_DIR)
