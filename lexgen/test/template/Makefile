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

PS_INC_PATH = $(INC_PATH)parser/
PS_PATH = $(SRC_PATH)parser/

LG_PATH = $(SRC_PATH)lexgen/

test.exe : template.c $(LG_PATH)template.c $(PS_PATH)hashmap/str_map.c
	$(CC) -o $@ $^ $(OPTIMIZE) $(CC_GLOBAL_PARAM)


,PHONY clean:
	rm *.exe
