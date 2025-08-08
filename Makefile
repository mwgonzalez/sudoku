CFLAGS = -Wall -O3 -march=haswell

all: solver baseline

clean:
	rm -f solver baseline wrap.o spin.o

solver: wrap.o solver.c
	gcc ${CFLAGS} -DBENCHMARK $^ -o $@

baseline: wrap.o spin.o
	gcc ${CFLAGS} $^ -o $@

wrap.o: wrap.c
	gcc ${CFLAGS} -c $^ -o $@

spin.o: spin.asm
	nasm -f elf64 $^ -o $@
