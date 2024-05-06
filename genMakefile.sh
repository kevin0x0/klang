#!/bin/bash

gen_deps() {
  local incdir="$1"
  local prefix="$2"
  local outputfile="$3"
  local command="$4"
  for file in $prefix*; do
    if [ -f $file ]; then
      local list=$(gcc -MM $file $incdir | tr ': \\' '\n')
      local target='$(OBJ_DIR)'$(echo $list | tr ' ' '\n' | awk '/\.o/{print $1}')
      local source='$(SRC_DIR)'$(echo $list | tr ' ' '\n' | awk '/\.c/{sub(/src\//, ""); print $1}')
      local headers=
      for header in $(echo $list | tr ': \\' '\n' | awk '/^deps\/k\/.*\.h/{sub(/deps\/k\//, "$(DEPS_K_DIR)"); print $1}'); do
        headers="$headers "$header
      done
      for header in $(echo $list | tr ': \\' '\n' | awk '/^include\/.*\.h/{sub(/include\//, "$(INC_DIR)"); print $1}'); do
        headers="$headers "$header
      done
      echo $target ':' $source $headers '|' create_dir >> $outputfile
      echo -e "\t$command\n" >> $outputfile
    else
      gen_deps "$incdir" $file/ $outputfile "$command"
    fi
  done
}

gen_deps_pic() {
  local incdir="$1"
  local prefix="$2"
  local outputfile="$3"
  local command="$4"
  for file in $prefix*; do
    if [ -f $file ]; then
      local list=$(gcc -MM $file $incdir | tr ': \\' '\n')
      local target='$(OBJ_DIR)'$(echo $list | tr ' ' '\n' | awk '/\.o/{sub(/\.o/, ".pic.o"); print $1}')
      local source='$(SRC_DIR)'$(echo $list | tr ' ' '\n' | awk '/.*\.c/{sub(/src\//, ""); print $1}')
      local headers=
      for header in $(echo $list | tr ': \\' '\n' | awk '/^deps\/k\/.*\.h/{sub(/deps\/k\//, "$(DEPS_K_DIR)"); print $1}'); do
        headers="$headers "$header
      done
      for header in $(echo $list | tr ': \\' '\n' | awk '/^include\/.*\.h/{sub(/include\//, "$(INC_DIR)"); print $1}'); do
        headers="$headers "$header
      done
      echo $target : $source $headers '|' create_dir >> $outputfile
      echo -e "\t$command\n" >> $outputfile
    else
      gen_deps_pic "$incdir" $file/ $outputfile "$command"
    fi
  done
}

cat Makefile.pre > Makefile

gen_deps '-I . -I deps/k/'  ./src/ Makefile '$(CC) -c -o $@ $< $(CFLAGS)'

echo -e "\n\n" >> Makefile
echo "# ==============================SHARED OBJECT==================================" >> Makefile
for dir in ast code error misc parse; do
  gen_deps_pic '-I . -I ./deps/k/' ./src/$dir Makefile '$(CC) -c -o $@ $< $(CFLAGS) -fPIC'
done

cat Makefile.post >> Makefile
