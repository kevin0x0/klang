ROOT_DIR = $(abspath ./)/

INC_DIR = $(ROOT_DIR)include/
SRC_DIR = $(ROOT_DIR)src/
LIB_DIR = $(ROOT_DIR)lib/

DEPS_K_DIR = $(ROOT_DIR)deps/k/


AR = ar rcs
CC = gcc
OPTIMIZE = -O2
CFLAGS = $(OPTIMIZE) -I $(ROOT_DIR) -I $(DEPS_K_DIR)

KLANG_OBJS = $(LIB_DIR)klapi.o $(LIB_DIR)klutils.o $(LIB_DIR)klgc.o $(LIB_DIR)klmm.o $(LIB_DIR)klarray.o $(LIB_DIR)klmap.o $(LIB_DIR)klclass.o $(LIB_DIR)klclosure.o \
       $(LIB_DIR)klkfunc.o $(LIB_DIR)klref.o $(LIB_DIR)klstring.o $(LIB_DIR)klvalue.o $(LIB_DIR)klcommon.o $(LIB_DIR)klexec.o $(LIB_DIR)klstack.o \
			 $(LIB_DIR)klstate.o $(LIB_DIR)klthrow.o

# KLANGC_OBJS = $(LIB_DIR)


all : $(LIB_DIR)libklang.a $(LIB_DIR)libklangc.a | create_lib_dir
	echo "done"

$(LIB_DIR)libklang.a : $(KLANG_OBJS) $(DEPS_K_DIR)lib/libk.a | create_lib_dir
	cp $(DEPS_K_DIR)lib/libk.a $(LIB_DIR)libklang.a
	$(AR) $@ $^

$(LIB_DIR)libklangc.a : $(KLANGC_OBJS) $(DEPS_K_DIR)lib/libk.a | create_lib_dir
	cp $(DEPS_K_DIR)lib/libk.a $(LIB_DIR)libklangc.a
	$(AR) $@ $^

$(DEPS_K_DIR)lib/libk.a :
	$(MAKE) -C $(DEPS_K_DIR) "CFLAGS=$(CFLAGS)" "CC=$(CC)"


# klang/klapi
$(LIB_DIR)klapi.o : $(SRC_DIR)klapi.c | create_lib_dir
	$(CC) -c -o $@ $< $(CFLAGS)

# klang/misc/
$(LIB_DIR)klutils.o : $(SRC_DIR)misc/klutils.c | create_lib_dir
	$(CC) -c -o $@ $< $(CFLAGS)

# klang/mm/
$(LIB_DIR)klgc.o : $(SRC_DIR)mm/klgc.c | create_lib_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(LIB_DIR)klmm.o : $(SRC_DIR)mm/klmm.c | create_lib_dir
	$(CC) -c -o $@ $< $(CFLAGS)

# klang/value/
$(LIB_DIR)klarray.o : $(SRC_DIR)value/klarray.c | create_lib_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(LIB_DIR)klmap.o : $(SRC_DIR)value/klmap.c | create_lib_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(LIB_DIR)klclass.o : $(SRC_DIR)value/klclass.c | create_lib_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(LIB_DIR)klclosure.o : $(SRC_DIR)value/klclosure.c | create_lib_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(LIB_DIR)klkfunc.o : $(SRC_DIR)value/klkfunc.c | create_lib_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(LIB_DIR)klref.o : $(SRC_DIR)value/klref.c | create_lib_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(LIB_DIR)klstring.o : $(SRC_DIR)value/klstring.c | create_lib_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(LIB_DIR)klvalue.o : $(SRC_DIR)value/klvalue.c | create_lib_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(LIB_DIR)klstate.o : $(SRC_DIR)value/klstate.c | create_lib_dir
	$(CC) -c -o $@ $< $(CFLAGS)


# klang/vm/
$(LIB_DIR)klcommon.o : $(SRC_DIR)vm/klcommon.c | create_lib_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(LIB_DIR)klexec.o : $(SRC_DIR)vm/klexec.c | create_lib_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(LIB_DIR)klstack.o : $(SRC_DIR)vm/klstack.c | create_lib_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(LIB_DIR)klthrow.o : $(SRC_DIR)vm/klthrow.c | create_lib_dir
	$(CC) -c -o $@ $< $(CFLAGS)






.PHONY: clean create_lib_dir
clean :
	$(RM) $(LIB_DIR)*
	$(MAKE) -C $(DEPS_K_DIR) clean

create_lib_dir :
	mkdir -p $(LIB_DIR)
