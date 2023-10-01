INC_DIR = $(TEMPL_DIR)include/
SRC_DIR = $(TEMPL_DIR)src/
LIB_DIR = $(TEMPL_DIR)lib/

UTILS_INC = $(UTILS_DIR)include/


$(LIB_DIR)libtemplate.a : $(LIB_DIR)template.o | create_lib_dir
	cp $(UTILS_DIR)lib/libutils.a $(LIB_DIR)libtemplate.a
	$(AR) $@ $^

$(LIB_DIR)template.o : $(SRC_DIR)template.c $(INC_DIR)template.h $(UTILS_INC)hashmap/str_map.h $(UTILS_INC)hashmap/func_map.h $(UTILS_INC)hashmap/strx_map.h | create_lib_dir
	$(CC) -c -o $@ $< $(CFLAGS) 

.PHONY: clean create_lib_dir
clean :
	$(RM) $(LIB_DIR)*

create_lib_dir :
	mkdir -p $(LIB_DIR)
