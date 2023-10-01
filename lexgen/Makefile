INC_DIR = $(LEXGEN_DIR)include/
SRC_DIR = $(LEXGEN_DIR)src/
BIN_DIR = $(LEXGEN_DIR)bin/

#parser path
PS_DIR = $(SRC_DIR)parser/
#lexgen path
LG_DIR = $(SRC_DIR)lexgen/

UTILS_LIB_DIR = $(UTILS_DIR)lib/
KEVFA_LIB_DIR = $(KEVFA_DIR)lib/
TEMPL_LIB_DIR = $(TEMPL_DIR)lib/

all : $(BIN_DIR)main.o | create_bin_dir
	$(MAKE) -C $(PS_DIR)
	$(MAKE) -C $(LG_DIR)
	$(CC) -o $(BIN_DIR)$(TARGET_LEXGEN_NAME) $(BIN_DIR)*.o -L $(KEVFA_LIB_DIR) -lkevfa -L $(UTILS_LIB_DIR) -lutils -L $(TEMPL_LIB_DIR) -ltemplate

$(BIN_DIR)main.o : $(SRC_DIR)main.c | create_bin_dir
	$(CC) -c -o $@ $< $(CFLAGS) 


.PHONY: clean create_bin_dir
clean :
	$(RM) $(BIN_DIR)*

create_bin_dir :
	mkdir -p $(BIN_DIR)
