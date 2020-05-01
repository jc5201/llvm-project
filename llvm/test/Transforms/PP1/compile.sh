file_name=$1
clang -O0 -Xclang -disable-O0-optnone -fno-discard-value-names -emit-llvm -c $file_name.cpp
llvm-dis $file_name.bc -o $file_name.ll
opt -mem2reg -S $file_name.ll -o $file_name-opt.ll

