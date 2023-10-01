INC_DIR = $(PARGEN_DIR)include/
SRC_DIR = $(PARGEN_DIR)src/
BIN_DIR = $(PARGEN_DIR)bin/

#parser path
PS_DIR = $(SRC_DIR)parser/
#pargen path
PG_DIR = $(SRC_DIR)pargen/

UTILS_LIB_DIR = $(UTILS_DIR)lib/
KEVLR_LIB_DIR = $(KEVLR_DIR)lib/
TEMPL_LIB_DIR = $(TEMPL_DIR)lib/

all : $(BIN_DIR)main.o | create_bin_dir
	$(MAKE) -C $(PS_DIR)
	$(MAKE) -C $(PG_DIR)
	$(CC) -o $(BIN_DIR)$(TARGET_PARGEN_NAME) $(BIN_DIR)*.o -L $(KEVLR_LIB_DIR) -lkevlr -L $(UTILS_LIB_DIR) -lutils -L $(TEMPL_LIB_DIR) -ltemplate

$(BIN_DIR)main.o : $(SRC_DIR)main.c | create_bin_dir
	$(CC) -c -o $@ $< $(CFLAGS) 


.PHONY: clean create_bin_dir
clean :
	$(RM) $(BIN_DIR)*

create_bin_dir :
	mkdir -p $(BIN_DIR)
