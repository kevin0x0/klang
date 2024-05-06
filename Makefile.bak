ROOT_DIR = $(abspath ./)/

INC_DIR = $(ROOT_DIR)include/
SRC_DIR = $(ROOT_DIR)src/
LIB_DIR = $(ROOT_DIR)lib/
OBJ_DIR = $(ROOT_DIR)obj/

DEPS_K_DIR = $(ROOT_DIR)deps/k/


AR = ar rcs
CC = gcc
OPTIMIZE = -O2
MEMORY_CHECK =
WARNING = -Wall -Wextra -Winline
DEBUG = -DNDEBUG

CFLAGS = $(OPTIMIZE) $(MEMORY_CHECK) $(WARNING) $(DEBUG) -I $(ROOT_DIR) -I $(DEPS_K_DIR)

KLANG_OBJS = $(OBJ_DIR)klapi.o $(OBJ_DIR)klutils.o $(OBJ_DIR)klgc.o $(OBJ_DIR)klmm.o $(OBJ_DIR)klarray.o $(OBJ_DIR)klmap.o \
						 $(OBJ_DIR)klclass.o $(OBJ_DIR)klclosure.o $(OBJ_DIR)klkfunc.o $(OBJ_DIR)klref.o $(OBJ_DIR)klstring.o \
						 $(OBJ_DIR)klvalue.o $(OBJ_DIR)klcommon.o $(OBJ_DIR)klexec.o $(OBJ_DIR)klstack.o $(OBJ_DIR)klstate.o $(OBJ_DIR)klthrow.o \
             $(OBJ_DIR)klconvert.o

KLANGC_OBJS = $(OBJ_DIR)klparser.o $(OBJ_DIR)kltokens.o $(OBJ_DIR)kllex.o $(OBJ_DIR)klidarr.o $(OBJ_DIR)klcfdarr.o \
							$(OBJ_DIR)klerror.o $(OBJ_DIR)klast_expr.o $(OBJ_DIR)klast_stmt.o $(OBJ_DIR)klstrtbl.o $(OBJ_DIR)klcode.o \
							$(OBJ_DIR)klcode_dump.o $(OBJ_DIR)klcode_print.o $(OBJ_DIR)klcontbl.o $(OBJ_DIR)klgen.o $(OBJ_DIR)klgen_expr.o \
							$(OBJ_DIR)klgen_exprbool.o $(OBJ_DIR)klgen_pattern.o $(OBJ_DIR)klgen_stmt.o $(OBJ_DIR)klsymtbl.o

KLANGC_PIC_OBJS = $(patsubst %.o, %.pic.o, $(KLANGC_OBJS))


all: $(LIB_DIR)libklang.a $(LIB_DIR)libklangc.a $(LIB_DIR)libklangc.so 


$(LIB_DIR)libklang.a : $(KLANG_OBJS) | create_dir
	$(AR) $@ $^

$(LIB_DIR)libklangc.a : $(KLANGC_OBJS) $(DEPS_K_DIR)lib/libk.a | create_dir
	cp $(DEPS_K_DIR)lib/libk.a $(OBJ_DIR)libklangc.a
	$(AR) $@ $^

$(LIB_DIR)libklangc.so : $(KLANGC_PIC_OBJS) $(DEPS_K_DIR)lib/libk.a | create_dir
	$(CC) -shared -o $@ $(KLANGC_PIC_OBJS) -L $(DEPS_K_DIR)lib/ -Wl,-Bstatic -lk -Wl,-Bdynamic


$(DEPS_K_DIR)lib/libk.a :
	$(MAKE) -C $(DEPS_K_DIR) $(DEPS_K_DIR)lib/libk.a

$(DEPS_K_DIR)lib/libk.so :
	$(MAKE) -C $(DEPS_K_DIR) $(DEPS_K_DIR)lib/libk.so


# ============================KLANG INTERPRETER================================


# klang/klapi/
$(OBJ_DIR)klapi.o : $(SRC_DIR)klapi.c | create_dir
	$(CC) -c -o $@ $< $(CFLAGS)

# klang/misc/
$(OBJ_DIR)klutils.o : $(SRC_DIR)misc/klutils.c | create_dir
	$(CC) -c -o $@ $< $(CFLAGS)

# klang/mm/
$(OBJ_DIR)klgc.o : $(SRC_DIR)mm/klgc.c | create_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(OBJ_DIR)klmm.o : $(SRC_DIR)mm/klmm.c | create_dir
	$(CC) -c -o $@ $< $(CFLAGS)

# klang/value/
$(OBJ_DIR)klarray.o : $(SRC_DIR)value/klarray.c | create_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(OBJ_DIR)klmap.o : $(SRC_DIR)value/klmap.c | create_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(OBJ_DIR)klclass.o : $(SRC_DIR)value/klclass.c | create_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(OBJ_DIR)klcoroutine.o : $(SRC_DIR)value/klcoroutine.c | create_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(OBJ_DIR)klclosure.o : $(SRC_DIR)value/klclosure.c | create_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(OBJ_DIR)klkfunc.o : $(SRC_DIR)value/klkfunc.c | create_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(OBJ_DIR)klref.o : $(SRC_DIR)value/klref.c | create_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(OBJ_DIR)klstring.o : $(SRC_DIR)value/klstring.c | create_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(OBJ_DIR)klvalue.o : $(SRC_DIR)value/klvalue.c | create_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(OBJ_DIR)klstate.o : $(SRC_DIR)value/klstate.c | create_dir
	$(CC) -c -o $@ $< $(CFLAGS)

# klang/vm/
$(OBJ_DIR)klcommon.o : $(SRC_DIR)vm/klcommon.c | create_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(OBJ_DIR)klexec.o : $(SRC_DIR)vm/klexec.c | create_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(OBJ_DIR)klstack.o : $(SRC_DIR)vm/klstack.c | create_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(OBJ_DIR)klthrow.o : $(SRC_DIR)vm/klthrow.c | create_dir
	$(CC) -c -o $@ $< $(CFLAGS)

# klang/lang/
$(OBJ_DIR)klconvert.o : $(SRC_DIR)lang/klconvert.c | create_dir
	$(CC) -c -o $@ $< $(CFLAGS)




# ===========================KLANG COMPILER====================================


# klang/parse/
$(OBJ_DIR)klparser.o : $(SRC_DIR)parse/klparser.c | create_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(OBJ_DIR)kltokens.o : $(SRC_DIR)parse/kltokens.c | create_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(OBJ_DIR)kllex.o : $(SRC_DIR)parse/kllex.c | create_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(OBJ_DIR)klidarr.o : $(SRC_DIR)parse/klidarr.c | create_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(OBJ_DIR)klcfdarr.o : $(SRC_DIR)parse/klcfdarr.c | create_dir
	$(CC) -c -o $@ $< $(CFLAGS)

# # klang/misc/
# $(OBJ_DIR)klutils.o : $(SRC_DIR)misc/klutils.c | create_dir
# 	$(CC) -c -o $@ $< $(CFLAGS)

# klang/error/
$(OBJ_DIR)klerror.o : $(SRC_DIR)error/klerror.c | create_dir
	$(CC) -c -o $@ $< $(CFLAGS)

# klang/ast/
$(OBJ_DIR)klast_expr.o : $(SRC_DIR)ast/klast_expr.c | create_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(OBJ_DIR)klast_stmt.o : $(SRC_DIR)ast/klast_stmt.c | create_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(OBJ_DIR)klstrtbl.o : $(SRC_DIR)ast/klstrtbl.c | create_dir
	$(CC) -c -o $@ $< $(CFLAGS)

# klang/code/
$(OBJ_DIR)klcode.o : $(SRC_DIR)code/klcode.c | create_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(OBJ_DIR)klcode_dump.o : $(SRC_DIR)code/klcode_dump.c | create_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(OBJ_DIR)klcode_print.o : $(SRC_DIR)code/klcode_print.c | create_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(OBJ_DIR)klcontbl.o : $(SRC_DIR)code/klcontbl.c | create_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(OBJ_DIR)klgen.o : $(SRC_DIR)code/klgen.c | create_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(OBJ_DIR)klgen_expr.o : $(SRC_DIR)code/klgen_expr.c | create_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(OBJ_DIR)klgen_exprbool.o : $(SRC_DIR)code/klgen_exprbool.c | create_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(OBJ_DIR)klgen_pattern.o : $(SRC_DIR)code/klgen_pattern.c | create_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(OBJ_DIR)klgen_stmt.o : $(SRC_DIR)code/klgen_stmt.c | create_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(OBJ_DIR)klsymtbl.o : $(SRC_DIR)code/klsymtbl.c | create_dir
	$(CC) -c -o $@ $< $(CFLAGS)


# ------------------------------SHARED OBJECT----------------------------------

# klang/parse/
$(OBJ_DIR)klparser.pic.o : $(SRC_DIR)parse/klparser.c | create_dir
	$(CC) -c -o $@ $< $(CFLAGS) -fPIC

$(OBJ_DIR)kltokens.pic.o : $(SRC_DIR)parse/kltokens.c | create_dir
	$(CC) -c -o $@ $< $(CFLAGS) -fPIC

$(OBJ_DIR)kllex.pic.o : $(SRC_DIR)parse/kllex.c | create_dir
	$(CC) -c -o $@ $< $(CFLAGS) -fPIC

$(OBJ_DIR)klidarr.pic.o : $(SRC_DIR)parse/klidarr.c | create_dir
	$(CC) -c -o $@ $< $(CFLAGS) -fPIC

$(OBJ_DIR)klcfdarr.pic.o : $(SRC_DIR)parse/klcfdarr.c | create_dir
	$(CC) -c -o $@ $< $(CFLAGS) -fPIC

# klang/misc/
$(OBJ_DIR)klutils.pic.o : $(SRC_DIR)misc/klutils.c | create_dir
	$(CC) -c -o $@ $< $(CFLAGS) -fPIC

# klang/error/
$(OBJ_DIR)klerror.pic.o : $(SRC_DIR)error/klerror.c | create_dir
	$(CC) -c -o $@ $< $(CFLAGS) -fPIC

# klang/ast/
$(OBJ_DIR)klast_expr.pic.o : $(SRC_DIR)ast/klast_expr.c | create_dir
	$(CC) -c -o $@ $< $(CFLAGS) -fPIC

$(OBJ_DIR)klast_stmt.pic.o : $(SRC_DIR)ast/klast_stmt.c | create_dir
	$(CC) -c -o $@ $< $(CFLAGS) -fPIC

$(OBJ_DIR)klstrtbl.pic.o : $(SRC_DIR)ast/klstrtbl.c | create_dir
	$(CC) -c -o $@ $< $(CFLAGS) -fPIC

# klang/code/
$(OBJ_DIR)klcode.pic.o : $(SRC_DIR)code/klcode.c | create_dir
	$(CC) -c -o $@ $< $(CFLAGS) -fPIC

$(OBJ_DIR)klcode_dump.pic.o : $(SRC_DIR)code/klcode_dump.c | create_dir
	$(CC) -c -o $@ $< $(CFLAGS) -fPIC

$(OBJ_DIR)klcode_print.pic.o : $(SRC_DIR)code/klcode_print.c | create_dir
	$(CC) -c -o $@ $< $(CFLAGS) -fPIC

$(OBJ_DIR)klcontbl.pic.o : $(SRC_DIR)code/klcontbl.c | create_dir
	$(CC) -c -o $@ $< $(CFLAGS) -fPIC

$(OBJ_DIR)klgen.pic.o : $(SRC_DIR)code/klgen.c | create_dir
	$(CC) -c -o $@ $< $(CFLAGS) -fPIC

$(OBJ_DIR)klgen_expr.pic.o : $(SRC_DIR)code/klgen_expr.c | create_dir
	$(CC) -c -o $@ $< $(CFLAGS) -fPIC

$(OBJ_DIR)klgen_exprbool.pic.o : $(SRC_DIR)code/klgen_exprbool.c | create_dir
	$(CC) -c -o $@ $< $(CFLAGS) -fPIC

$(OBJ_DIR)klgen_pattern.pic.o : $(SRC_DIR)code/klgen_pattern.c | create_dir
	$(CC) -c -o $@ $< $(CFLAGS) -fPIC

$(OBJ_DIR)klgen_stmt.pic.o : $(SRC_DIR)code/klgen_stmt.c | create_dir
	$(CC) -c -o $@ $< $(CFLAGS) -fPIC

$(OBJ_DIR)klsymtbl.pic.o : $(SRC_DIR)code/klsymtbl.c | create_dir
	$(CC) -c -o $@ $< $(CFLAGS) -fPIC




.PHONY: clean create_dir all
clean :
	$(RM) -r $(OBJ_DIR)* $(LIB_DIR)*
	$(MAKE) -C $(DEPS_K_DIR) clean

create_dir :
	mkdir -p $(OBJ_DIR) $(LIB_DIR)
