#!/usr/bin/env bash

set -e
if [ ! -f run.bash ]; then
	echo 'run.bash must be run from bad/test' 1>&2
	exit 1
fi

mkdir -p bin

FLAGS="-g -I.. -Werror -Wall -Wpadded -Winline -Wconversion -Wcast-align"

if [ "$1" == "-v" ]; then
	set -x
fi

cc -o bin/build_c_test $FLAGS build_c_test.c
./bin/build_c_test

cc -o bin/build_cpp_test $FLAGS build_cpp_test.cpp
./bin/build_cpp_test

cc -o bin/terminal_test $FLAGS terminal_test.cpp
./bin/terminal_test
