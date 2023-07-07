OPTIMIZE = -g
export CC = gcc
export AR = ar rcs
export ROOT_PATH = $(abspath $(shell pwd))/
export LEXGEN_PATH = $(ROOT_PATH)lexgen/
export PARGEN_PATH = $(ROOT_PATH)pargen/
export UTILS_PATH = $(ROOT_PATH)utils/
export CFLAGS = -Wall $(OPTIMIZE) -I $(ROOT_PATH)
export TARGET_LEXGEN_NAME = lexgen.exe
export TARGET_PARGEN_NAME = pargen.exe

LEXGEN_PATH = $(ROOT_PATH)lexgen/
UTILS_PATH = $(ROOT_PATH)utils/
PARGEN_PATH = $(ROOT_PATH)pargen/

all :
	$(MAKE) -C $(UTILS_PATH)
	$(MAKE) -C $(LEXGEN_PATH)

lexgen :
	$(MAKE) -C $(UTILS_PATH)
	$(MAKE) -C $(LEXGEN_PATH)

pargen :
	$(MAKE) -C $(UTILS_PATH)
	$(MAKE) -C $(PARGEN_PATH)

.PHONY: clean
clean :
	$(MAKE) -C $(UTILS_PATH) clean
	$(MAKE) -C $(LEXGEN_PATH) clean
