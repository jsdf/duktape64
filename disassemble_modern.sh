#!/bin/bash
set -eu
 
 
executable_file="squaresdemo.out"

# # pass program counter address as arg to this script to see disassembly around
# # the program counter
# pc_address="${1:-''}"


mips64-elf-objdump  -S --syms  -m mips:4300  --prefix-addresses "$executable_file" > disassembly.txt

# if [ -n "$pc_address" ]; then 
#   grep --color=always -C 8 "${pc_address/0x/}" disassembly.txt
# fi 



i=0
for arg in "${@:1}"; do

  if [[ "$i" = "0" ]]; then
    echo "exception at:"
    grep --color=always -C 8 "^${arg/0x/} <\w*\W" disassembly.txt
  else
    echo "called from:"
    grep --color=always  "^${arg/0x/} <\w*\W" disassembly.txt
  fi

    ((i=i+1))
done
