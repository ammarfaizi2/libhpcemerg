# SPDX-License-Identifier: GPL-2.0-only

INCLUDE_FLAGS = -Isrc/include
CFLAGS = $(INCLUDE_FLAGS) -pg -mfentry -O3

all: main

src/arch/x86/entry/entry_64.o: src/arch/x86/entry/entry_64.S

target.o: target.c
	$(CC) $(INCLUDE_FLAGS) -O3 -o $(@) -c $(^)

main.o: main.c src/include/libhpcemerg.h

main: src/arch/x86/entry/entry_64.o main.o target.o

clean:
	rm -vf src/arch/x86/entry/entry_64.o main.o target.o;
