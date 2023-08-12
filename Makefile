OPTIMIZE = -g
export CC = gcc
export AR = ar rcs
export ROOT_DIR = $(abspath $(shell pwd))/
export LEXGEN_DIR = $(ROOT_DIR)lexgen/
export PARGEN_DIR = $(ROOT_DIR)pargen/
export UTILS_DIR = $(ROOT_DIR)utils/
export CFLAGS = -Wall $(OPTIMIZE) -I $(ROOT_DIR)
export TARGET_LEXGEN_NAME = lexgen.exe
export TARGET_PARGEN_NAME = pargen.exe

all :
	$(MAKE) -C $(UTILS_DIR)
	$(MAKE) -C $(LEXGEN_DIR)
	$(MAKE) -C $(PARGEN_DIR)

.PHONY: clean
clean :
	$(MAKE) -C $(UTILS_DIR) clean
	$(MAKE) -C $(LEXGEN_DIR) clean
	$(MAKE) -C $(PARGEN_DIR) clean
