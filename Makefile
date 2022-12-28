.phony all:
all: mts

mts: mts.c
	gcc mts.c -o mts -lpthread

.PHONY clean:
clean:
	-rm -rf *.o *.exe
