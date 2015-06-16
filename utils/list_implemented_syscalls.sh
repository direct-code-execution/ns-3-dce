#!/bin/bash
# this script should display one implemented syscall per line
# it assumes libc-ns3.h only implements one syscall per line
# this line removes C/C++ comments
file="$(dirname ${BASH_SOURCE:-$0})/../model/libc-ns3.h"

if [ $# -gt 1 ]; then
	file="$1"
fi

# preprocess file with gcc to remove C/C++ comments
# use 'lookaround' regex patterns to select name within parentheses
# http://unix.stackexchange.com/questions/103004/grep-regex-only-for-match-anything-between-parenthesis
gcc -fpreprocessed -E "$file" | grep -E "DCE|NATIVE" | grep -Po '(?<=\().*(?=\))' 
