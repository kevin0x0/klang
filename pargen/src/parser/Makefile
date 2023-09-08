INC_DIR = $(PARGEN_DIR)include/
SRC_DIR = $(PARGEN_DIR)src/
BIN_DIR = $(PARGEN_DIR)bin/

#parser path
PS_INC_DIR = $(INC_DIR)parser/
PS_DIR = $(SRC_DIR)parser/

UTILS_INC = $(UTILS_DIR)include/
GEN_INC_DIR = $(UTILS_INC)general/

all : $(BIN_DIR)error.o $(BIN_DIR)lexer.o $(BIN_DIR)lextab.o


$(BIN_DIR)error.o : $(PS_DIR)error.c $(PS_INC_DIR)error.h $(GEN_INC_DIR)global_def.h
	$(CC) -c -o $@ $< $(CFLAGS)

$(BIN_DIR)lexer.o : $(PS_DIR)lexer.c $(PS_INC_DIR)lexer.h $(PS_INC_DIR)partokens.h $(GEN_INC_DIR)global_def.h
	$(CC) -c -o $@ $< $(CFLAGS)

$(BIN_DIR)lextab.o : $(PS_DIR)lextab.c $(PS_INC_DIR)error.h $(PS_INC_DIR)lexer.h $(UTILS_INC)string/kev_string.h
	$(CC) -c -o $@ $< $(CFLAGS)
