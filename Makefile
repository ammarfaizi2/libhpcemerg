# SPDX-License-Identifier: GPL-2.0


CC = clang
CXX = clang++
CFLAGS = -Isrc -Wall -Wextra -ggdb3 -fpic -fPIC -O0 -Wno-unused-parameter
OBJ_CC := main.o

include src/Makefile

all: main

main: $(OBJ_CC)
	$(CC) -rdynamic $(OBJ_CC) -o $(@) -ldl

clean:
	rm -vf main main.o $(OBJ_CC)

.PHONY: all
