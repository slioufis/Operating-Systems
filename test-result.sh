#!/bin/sh

gcc -pthread -g -o pizza  pizza.c
./pizza 100 10
rm pizza

# valgrind -s ./test-result.sh produces memory leaks for some reason
# whereas
# valgrind -s ./pizza <arg1> <arg2> (provided we excplicity compiled the program with 'gcc -pthread -g -o pizza  filename.c') does not