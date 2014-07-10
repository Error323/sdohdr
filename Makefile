CC=g++
CFLAGS= -O3 -Wall -Wextra -std=c++0x -march=native
DBGFLAGS= -O2 -g -Wall -Wextra -std=c++0x -march=native
LDFLAGS=-lrt

all:
	$(CC) -o sdohdr $(CFLAGS) sdohdr.cc $(LDFLAGS)

dbg:
	$(CC) -o sdohdr $(DBGFLAGS) sdohdr.cc $(LDFLAGS)
