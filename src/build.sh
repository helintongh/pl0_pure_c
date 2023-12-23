#!/bin/sh
if [ ! -d "build" ]; then
    mkdir build
fi
cd ./build
cmake ../pl0_compiler
make

# 测试代码
echo PL/0 compiler test suite
echo ========================

for i in ../tests/*.pl0 ; do
  /usr/bin/printf "%.13s... " $i
  ./pl0_c $i > /dev/null 2>&1
  if [ $? -eq 0 ] ; then
    echo ok
  else
    echo fail
  fi
done