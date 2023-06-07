#!/bin/sh
clear
libraries="-lm -pthread"
flags="-std=c11 -Wall -Wextra -Wpedantic -Werror -Ofast"
# flags="-std=c11 -Wall -Wextra -Wpedantic -Werror -g"
gcc main.c $flags $libraries -o main.out
