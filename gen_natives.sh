#!/usr/bin/zsh
# goal is to generate libcns3.h
#
# foulle dans liste de fichiers
# list=(dce-termio.h dce-math.h dce-socket.h dce-fd.h)
out="natives.h"

# sys/dce-socket.h 
# read '' var <<'EOF'
# EOF
echo "/*THIS FILE WAS GENEREATED by $0*/" > "$out"
echo "#define NATIVE(name,...) name" >> "$out"
# echo "$VAR" > "$out"
# files="model/sys/dce-socket.h"
# shopt -s globstar
# could generate libdl / libm
files=(model/**.h)
grep --exclude="libc-ns3.h" --no-filename -e "^NATIVE *(" $files >> "$out"
gcc "$out" -E > "$out.txt"
cat "$out.txt"
