INC_DIR = $(LEXGEN_DIR)include/
SRC_DIR = $(LEXGEN_DIR)src/
BIN_DIR = $(LEXGEN_DIR)bin/

#parser path
PS_INC_DIR = $(INC_DIR)parser/
PS_DIR = $(SRC_DIR)parser/
#lexgen path
LG_INC_DIR = $(INC_DIR)lexgen/
LG_DIR = $(SRC_DIR)lexgen/

UTILS_INC = $(UTILS_DIR)include/

all : $(BIN_DIR)cmdline.o $(BIN_DIR)control.o $(BIN_DIR)output.o $(BIN_DIR)convert.o $(BIN_DIR)lg_error.o $(BIN_DIR)dir.o

$(BIN_DIR)cmdline.o : $(LG_DIR)cmdline.c $(LG_INC_DIR)cmdline.h $(LG_INC_DIR)options.h $(LG_INC_DIR)error.h
	$(CC) -c -o $@ $< $(CFLAGS) 

$(BIN_DIR)control.o : $(LG_DIR)control.c $(LG_INC_DIR)control.h $(LG_INC_DIR)options.h $(PS_INC_DIR)parser.h $(PS_INC_DIR)hashmap/strfa_map.h $(PS_INC_DIR)list/pattern_list.h $(LG_INC_DIR)error.h $(UTILS_INC)hashmap/func_map.h $(UTILS_INC)hashmap/strx_map.h
	$(CC) -c -o $@ $< $(CFLAGS) 

$(BIN_DIR)output.o : $(LG_DIR)output.c $(LG_INC_DIR)output.h $(LG_INC_DIR)options.h $(PS_INC_DIR)hashmap/strfa_map.h $(PS_INC_DIR)parser.h $(PS_INC_DIR)hashmap/strfa_map.h $(PS_INC_DIR)list/pattern_list.h $(LG_INC_DIR)error.h $(UTILS_INC)string/kev_string.h
	$(CC) -c -o $@ $< $(CFLAGS) 

$(BIN_DIR)convert.o : $(LG_DIR)convert.c $(LG_INC_DIR)convert.h $(PS_INC_DIR)hashmap/strfa_map.h $(PS_INC_DIR)parser.h $(PS_INC_DIR)list/pattern_list.h $(LG_INC_DIR)error.h
	$(CC) -c -o $@ $< $(CFLAGS) 

$(BIN_DIR)lg_error.o : $(LG_DIR)error.c $(LG_INC_DIR)error.h
	$(CC) -c -o $@ $< $(CFLAGS) 

$(BIN_DIR)dir.o : $(LG_DIR)dir.c $(LG_INC_DIR)dir.h $(UTILS_INC)os_spec/dir.h
	$(CC) -c -o $@ $< $(CFLAGS) 
