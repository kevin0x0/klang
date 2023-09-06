
OPTIMIZE = -g
CC = gcc
AR = ar


ROOT_DIR = $(abspath ../../)/

CC_GLOBAL_PARAM = -Wall -I $(ROOT_DIR)

KEVLR_DIR = $(ROOT_DIR)kevlr/
INC_PATH = $(KEVLR_DIR)include/
SRC_PATH = $(KEVLR_DIR)src/
LIB_PATH = $(KEVLR_DIR)lib/

UTILS_DIR = $(ROOT_DIR)utils/

LR_INC_PATH = $(INC_PATH)lr/
LR_PATH = $(SRC_PATH)lr/

kevlr_test.exe : kevlr_test.c $(LIB_PATH)libkevlr.a
	$(CC) -o $@ $^ $(OPTIMIZE) $(CC_GLOBAL_PARAM)


,PHONY clean:
	rm *.exe
