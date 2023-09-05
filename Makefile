OPTIMIZE = -g
export CC = gcc
export AR = ar rcs
export ROOT_DIR = $(abspath $(shell pwd))/
export KEVFA_DIR = $(ROOT_DIR)kevfa/
export KEVLR_DIR = $(ROOT_DIR)kevlr/
export LEXGEN_DIR = $(ROOT_DIR)lexgen/
export PARGEN_DIR = $(ROOT_DIR)pargen/
export UTILS_DIR = $(ROOT_DIR)utils/
export CFLAGS = -Wall $(OPTIMIZE) -I $(ROOT_DIR)
export TARGET_LEXGEN_NAME = lexgen.exe
export TARGET_PARGEN_NAME = pargen.exe

all : lexgen_t pargen_t kevfa_t kevlr_t utils_t

lexgen_t : kevfa_t
	$(MAKE) -C $(LEXGEN_DIR)

pargen_t : kevlr_t
	$(MAKE) -C $(PARGEN_DIR)

kevfa_t : utils_t
	$(MAKE) -C $(KEVFA_DIR)

kevlr_t : utils_t
	$(MAKE) -C $(KEVLR_DIR)

utils_t :
	$(MAKE) -C $(UTILS_DIR)

.PHONY: clean
clean :
	$(MAKE) -C $(UTILS_DIR) clean
	$(MAKE) -C $(LEXGEN_DIR) clean
	$(MAKE) -C $(PARGEN_DIR) clean
	$(MAKE) -C $(KEVFA_DIR) clean
	$(MAKE) -C $(KEVLR_DIR) clean
