INC_PATH = $(LEXGEN_PATH)include/
SRC_PATH = $(LEXGEN_PATH)src/
BIN_PATH = $(LEXGEN_PATH)bin/
LIB_PATH = $(LEXGEN_PATH)lib/

#finite_automaton path
FA_PATH = $(SRC_PATH)finite_automaton/
#parser path
PS_PATH = $(SRC_PATH)parser/
#tablegen path
TG_PATH = $(SRC_PATH)tablegen/
#lexgen path
LG_PATH = $(SRC_PATH)lexgen/

all: $(BIN_PATH)main.o 
	mkdir -p $(BIN_PATH)
	$(MAKE) -C $(FA_PATH)
	$(MAKE) -C $(PS_PATH)
	$(MAKE) -C $(LG_PATH)
	$(CC) -o $(BIN_PATH)$(TARGET_LEXGEN_NAME) $(BIN_PATH)main.o $(BIN_PATH)liblexgen.a $(BIN_PATH)libparser.a $(BIN_PATH)libfa.a $(UTILS_PATH)bin/libutils.a

$(BIN_PATH)main.o : $(SRC_PATH)main.c
	mkdir -p $(BIN_PATH)
	$(CC) -c -o $@ $< $(CFLAGS) 

.PHONY: clean
clean :
	rm $(BIN_PATH)*
