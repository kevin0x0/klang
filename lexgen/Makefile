INC_DIR = $(LEXGEN_DIR)include/
SRC_DIR = $(LEXGEN_DIR)src/
BIN_DIR = $(LEXGEN_DIR)bin/

#parser path
PS_DIR = $(SRC_DIR)parser/
#lexgen path
LG_DIR = $(SRC_DIR)lexgen/

UTILS_LIB_DIR = $(UTILS_DIR)lib/
KEVFA_LIB_DIR = $(KEVFA_DIR)lib/

$(BIN_DIR)$(TARGET_LEXGEN_NAME) : $(BIN_DIR)main.o | create_bin_dir
	$(MAKE) -C $(PS_DIR)
	$(MAKE) -C $(LG_DIR)
	$(CC) -o $@ $(BIN_DIR)*.o $(KEVFA_LIB_DIR)libkevfa.a $(UTILS_LIB_DIR)libutils.a

$(BIN_DIR)main.o : $(SRC_DIR)main.c | create_bin_dir
	$(CC) -c -o $@ $< $(CFLAGS) 


.PHONY: clean create_bin_dir
clean :
	rm $(BIN_DIR)*

create_bin_dir :
	mkdir -p $(BIN_DIR)
