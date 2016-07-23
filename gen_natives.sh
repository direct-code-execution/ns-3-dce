#!/usr/bin/zsh
# goal is to generate libcns3.h
#
# foulle dans liste de fichiers
# list=(dce-termio.h dce-math.h dce-socket.h dce-fd.h)
out="libc-ns3.generated.h"
input="model/libc-ns3.h"
# out="libc-ns3.h"

# sys/dce-socket.h 
# read '' var <<'EOF'
# EOF
# echo "/*THIS FILE WAS GENEREATED by $0*/" > "$out"
# echo "#define NATIVE(name,...) name" >> "$out"
# echo "$VAR" > "$out"
# files="model/sys/dce-socket.h"
# shopt -s globstar
# could generate libdl / libm
# files=(model/**.h)
# todo from a specific file in particular
# FILTER=" --exclude='libc-ns3.h' -e '^NATIVE *(' $files"

# grep --no-filename $FILTER >> "$out"

# the -P prevents addition of preprocessor comments (from which file etc...)
# this generates a csvfiles
gcc "model/libc-ns3.h" -E -P -D'NATIVE(name,...)=native,name,__VA_ARGS__' -D'DCE(name,...)=dce,name,__VA_ARGS__' \
	-D'ALIAS(name,aliasname)=alias,name,aliasname'  \
> "$out.tmp"
# cat "$out.tmp"
echo "File saved in $out.txt"
