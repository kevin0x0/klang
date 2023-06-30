export OPTIMIZE = -g
export CC = gcc
export AR = ar
export AR_OPTION = rcs
export ROOT_PATH = $(abspath $(shell pwd)/..)/
export CC_GLOBAL_PARAM = -Wall -I $(ROOT_PATH)
TOKENIZER_PATH = $(ROOT_PATH)tokenizer_generator/
INC_PATH = $(TOKENIZER_PATH)include/
SRC_PATH = $(TOKENIZER_PATH)src/
BIN_PATH = $(TOKENIZER_PATH)bin/
LIB_PATH = $(TOKENIZER_PATH)lib/

#finite_automaton path
FA_PATH = $(SRC_PATH)finite_automaton/
#regex_parser path
RP_PATH = $(SRC_PATH)regex_parser/

all : 
	$(MAKE) -C $(FA_PATH)
	$(MAKE) -C $(RP_PATH)


.PHONY: clean

clean :
	-rm $(BIN_PATH)*.o $(BIN_PATH)*.a
