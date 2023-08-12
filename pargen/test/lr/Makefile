
OPTIMIZE = -g
CC = gcc
AR = ar


ROOT_DIR = $(abspath ../../../)/

CC_GLOBAL_PARAM = -Wall -I $(ROOT_DIR)

PARGEN_DIR = $(ROOT_DIR)pargen/
INC_PATH = $(PARGEN_DIR)include/
SRC_PATH = $(PARGEN_DIR)src/
BIN_PATH = $(PARGEN_DIR)bin/
LIB_PATH = $(PARGEN_DIR)lib/

UTILS_DIR = $(ROOT_DIR)utils/

LR_INC_PATH = $(INC_PATH)lr/
LR_PATH = $(SRC_PATH)lr/

lalr_test.exe : lalr_test.c $(BIN_PATH)liblr.a $(UTILS_DIR)bin/libutils.a
	$(CC) -o $@ $^ $(OPTIMIZE) $(CC_GLOBAL_PARAM)


,PHONY clean:
	rm *.exe
