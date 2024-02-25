test1 :
	gcc test1.c ../../src/kio/*.c ../../src/os_spec/file.c -I ../../../ -o test1 -g -O2

test2 :
	gcc test2.c ../../src/kio/*.c ../../src/os_spec/file.c -I ../../../ -o test2 -g -O2

.PHONY : clean

clean :
	$(RM) test1.exe test2.exe
