INC_DIR = $(LEXGEN_DIR)include/
SRC_DIR = $(LEXGEN_DIR)src/
BIN_DIR = $(LEXGEN_DIR)bin/
LIB_DIR = $(LEXGEN_DIR)lib/

#finite_automaton path
FA_DIR = $(SRC_DIR)finite_automaton/
#parser path
PS_DIR = $(SRC_DIR)parser/
#tablegen path
TG_DIR = $(SRC_DIR)tablegen/
#lexgen path
LG_DIR = $(SRC_DIR)lexgen/

all: $(BIN_DIR)main.o 
	mkdir -p $(BIN_DIR)
	$(MAKE) -C $(FA_DIR)
	$(MAKE) -C $(PS_DIR)
	$(MAKE) -C $(LG_DIR)
	$(CC) -o $(BIN_DIR)$(TARGET_LEXGEN_NAME) $(BIN_DIR)main.o $(BIN_DIR)liblexgen.a $(BIN_DIR)libparser.a $(BIN_DIR)libfa.a $(UTILS_DIR)bin/libutils.a

$(BIN_DIR)main.o : $(SRC_DIR)main.c
	mkdir -p $(BIN_DIR)
	$(CC) -c -o $@ $< $(CFLAGS) 

.PHONY: clean
clean :
	rm $(BIN_DIR)*
