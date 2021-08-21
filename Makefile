# SPDX-License-Identifier: GPL-2.0-only


CC = clang
CXX = clang++
CLANG_EXTRA = -Wno-padded -Wno-gnu-statement-expression -Wno-disabled-macro-expansion -Wno-missing-prototypes
CFLAGS = -Weverything -Isrc -Wall -Wextra -ggdb3 -fpic -fPIC -O0 -Wno-unused-parameter $(CLANG_EXTRA)
OBJ_CC := main.o

include src/Makefile

all: main

main: $(OBJ_CC)
	$(CC) -rdynamic $(OBJ_CC) -o $(@) -ldl

clean:
	rm -vf main main.o $(OBJ_CC)

.PHONY: all
