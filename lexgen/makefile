export OPTIMIZE = -g
export CC = gcc
export AR = ar
export AR_OPTION = rcs
export ROOT_PATH = $(abspath $(shell pwd)/..)/
export CC_GLOBAL_PARAM = -Wall -I $(ROOT_PATH)

EXEC_NAME = lexgen
LEXGEN_PATH = $(ROOT_PATH)lexgen/
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
	$(MAKE) -C $(FA_PATH)
	$(MAKE) -C $(PS_PATH)
	$(MAKE) -C $(TG_PATH)
	$(MAKE) -C $(LG_PATH)
	$(CC) -o $(EXEC_NAME) $(BIN_PATH)main.o $(BIN_PATH)lexgen.a $(BIN_PATH)tablegen.a $(BIN_PATH)parser.a $(BIN_PATH)finite_automaton.a

$(BIN_PATH)main.o : $(SRC_PATH)main.c
	$(CC) -c -o $@ $< $(OPTIMIZE) $(CC_GLOBAL_PARAM) 


.PHONY: clean

clean :
	-rm $(BIN_PATH)*.o $(BIN_PATH)*.a
