OPTIMIZE = -g
CC = gcc
AR = ar


ROOT_PATH = $(abspath ../../../)/

CC_GLOBAL_PARAM = -Wall -I $(ROOT_PATH)

LEXGEN_PATH = $(ROOT_PATH)lexgen/
INC_PATH = $(LEXGEN_PATH)include/
SRC_PATH = $(LEXGEN_PATH)src/
BIN_PATH = $(LEXGEN_PATH)bin/
LIB_PATH = $(LEXGEN_PATH)lib/

GEN_INC_PATH = $(INC_PATH)general/

#finite_automata path
FA_INC_PATH = $(INC_PATH)finite_automaton/
FA_PATH = $(SRC_PATH)finite_automaton/

RP_INC_PATH = $(INC_PATH)parser/
RP_PATH = $(SRC_PATH)parser/

TD_INC_PATH = $(INC_PATH)table_driven_dfa/
TD_PATH = $(SRC_PATH)table_driven_dfa/

test.exe : test.c $(RP_PATH)regex.c $(BIN_PATH)libfa.a $(RP_PATH)hashmap/strfa_map.c $(TD_PATH)table_driven_dfa.c
	$(CC) -o $@ $^ $(OPTIMIZE) $(CC_GLOBAL_PARAM)


,PHONY clean:
	rm *.exe
