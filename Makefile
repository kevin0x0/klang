DEBUGINFO = -g
OPTIMIZE =
export CC = gcc
export AR = ar rcs
export ROOT_DIR = $(abspath $(shell pwd))/
export KEVFA_DIR = $(ROOT_DIR)kevfa/
export KEVLR_DIR = $(ROOT_DIR)kevlr/
export KLANG_DIR = $(ROOT_DIR)klang/
export LEXGEN_DIR = $(ROOT_DIR)lexgen/
export PARGEN_DIR = $(ROOT_DIR)pargen/
export UTILS_DIR = $(ROOT_DIR)utils/
export TEMPL_DIR = $(ROOT_DIR)template/
export CFLAGS = -Wall -Wextra -Winline -pedantic $(OPTIMIZE) $(DEBUGINFO) -I $(ROOT_DIR)
export TARGET_LEXGEN_NAME = lexgen
export TARGET_PARGEN_NAME = pargen

all : lexgen_t pargen_t kevfa_t kevlr_t klang_t utils_t

lexgen_t : kevfa_t
	$(MAKE) -C $(LEXGEN_DIR)

pargen_t : kevlr_t
	$(MAKE) -C $(PARGEN_DIR)

kevfa_t : utils_t template_t
	$(MAKE) -C $(KEVFA_DIR)

klang_t : utils_t
	$(MAKE) -C $(KLANG_DIR)

kevlr_t : utils_t template_t
	$(MAKE) -C $(KEVLR_DIR)

template_t : utils_t
	$(MAKE) -C $(TEMPL_DIR)

utils_t : 
	$(MAKE) -C $(UTILS_DIR)


.PHONY: clean
clean :
	$(MAKE) -C $(UTILS_DIR) clean
	$(MAKE) -C $(LEXGEN_DIR) clean
	$(MAKE) -C $(PARGEN_DIR) clean
	$(MAKE) -C $(KEVFA_DIR) clean
	$(MAKE) -C $(KEVLR_DIR) clean
	$(MAKE) -C $(KLANG_DIR) clean
	$(MAKE) -C $(TEMPL_DIR) clean
