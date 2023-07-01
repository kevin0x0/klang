export OPTIMIZE = -g
export CC = gcc
export AR = ar
export AR_OPTION = rcs
export ROOT_PATH = $(abspath $(shell pwd)/..)/
export CC_GLOBAL_PARAM = -Wall -I $(ROOT_PATH)
LEXGEN_PATH = $(ROOT_PATH)lexgen/
INC_PATH = $(LEXGEN_PATH)include/
SRC_PATH = $(LEXGEN_PATH)src/
BIN_PATH = $(LEXGEN_PATH)bin/
LIB_PATH = $(LEXGEN_PATH)lib/

#finite_automaton path
FA_PATH = $(SRC_PATH)finite_automaton/
#regex_parser path
RP_PATH = $(SRC_PATH)parser/
#table_driven_dfa path
TD_PATH = $(SRC_PATH)table_driven_dfa/

all : 
	$(MAKE) -C $(FA_PATH)
	$(MAKE) -C $(RP_PATH)
	$(MAKE) -C $(TD_PATH)


.PHONY: clean

clean :
	-rm $(BIN_PATH)*.o $(BIN_PATH)*.a
