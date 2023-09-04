INC_DIR = $(LEXGEN_DIR)include/
SRC_DIR = $(LEXGEN_DIR)src/
BIN_DIR = $(LEXGEN_DIR)bin/
LIB_DIR = $(LEXGEN_DIR)lib/

GEN_INC_DIR = $(INC_DIR)general/

#finite_automaton path
FA_INC_DIR = $(INC_DIR)finite_automaton/
FA_DIR = $(SRC_DIR)finite_automaton/
#parser path
PS_INC_DIR = $(INC_DIR)parser/
PS_DIR = $(SRC_DIR)parser/
#lexgen path
LG_INC_DIR = $(INC_DIR)lexgen/
LG_DIR = $(SRC_DIR)lexgen/

UTILS_INC = $(UTILS_DIR)include/

FA_DEPS = $(FA_INC_DIR)finite_automaton.h $(FA_INC_DIR)graph.h

$(BIN_DIR)liblexgen.a : $(BIN_DIR)cmdline.o $(BIN_DIR)control.o $(BIN_DIR)template.o $(BIN_DIR)output.o $(BIN_DIR)convert.o $(BIN_DIR)lg_error.o
	$(AR) $@ $^

$(BIN_DIR)cmdline.o : $(LG_DIR)cmdline.c $(LG_INC_DIR)cmdline.h $(LG_INC_DIR)options.h $(LG_INC_DIR)error.h
	$(CC) -c -o $@ $< $(CFLAGS) 

$(BIN_DIR)control.o : $(LG_DIR)control.c $(LG_INC_DIR)control.h $(LG_INC_DIR)options.h $(PS_INC_DIR)parser.h $(PS_INC_DIR)hashmap/strfa_map.h $(PS_INC_DIR)list/pattern_list.h $(FA_DEPS) $(LG_INC_DIR)error.h
	$(CC) -c -o $@ $< $(CFLAGS) 

$(BIN_DIR)output.o : $(LG_DIR)output.c $(LG_INC_DIR)output.h $(LG_INC_DIR)options.h $(PS_INC_DIR)hashmap/strfa_map.h $(PS_INC_DIR)parser.h $(PS_INC_DIR)hashmap/strfa_map.h $(PS_INC_DIR)list/pattern_list.h $(LG_INC_DIR)template.h $(FA_DEPS) $(LG_INC_DIR)error.h $(UTILS_INC)string/kev_srting.h
	$(CC) -c -o $@ $< $(CFLAGS) 

$(BIN_DIR)template.o : $(LG_DIR)template.c $(LG_INC_DIR)template.h $(UTILS_INC)hashmap/str_map.h $(LG_INC_DIR)options.h $(LG_INC_DIR)error.h
	$(CC) -c -o $@ $< $(CFLAGS) 

$(BIN_DIR)convert.o : $(LG_DIR)convert.c $(LG_INC_DIR)convert.h $(PS_INC_DIR)hashmap/strfa_map.h $(PS_INC_DIR)parser.h $(PS_INC_DIR)list/pattern_list.h $(LG_INC_DIR)template.h $(FA_DEPS) $(LG_INC_DIR)error.h
	$(CC) -c -o $@ $< $(CFLAGS) 

$(BIN_DIR)lg_error.o : $(LG_DIR)error.c $(LG_INC_DIR)error.h
	$(CC) -c -o $@ $< $(CFLAGS) 
