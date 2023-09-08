INC_DIR = $(PARGEN_DIR)include/
SRC_DIR = $(PARGEN_DIR)src/
BIN_DIR = $(PARGEN_DIR)bin/


UTILS_LIB_DIR = $(UTILS_DIR)lib/
KEVLR_LIB_DIR = $(KEVLR_DIR)lib/

$(BIN_DIR)$(TARGET_PARGEN_NAME) : $(BIN_DIR)main.o | create_bin_dir
	$(CC) -o $@ $(BIN_DIR)*.o -L $(KEVLR_LIB_DIR) -lkevlr -L $(UTILS_LIB_DIR) -lutils

$(BIN_DIR)main.o : $(SRC_DIR)main.c | create_bin_dir
	$(CC) -c -o $@ $< $(CFLAGS) 


.PHONY: clean create_bin_dir
clean :
	$(RM) $(BIN_DIR)*

create_bin_dir :
	mkdir -p $(BIN_DIR)
