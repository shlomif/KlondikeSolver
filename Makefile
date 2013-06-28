all: KlondikeSolver

CFLAGS = -g

KlondikeSolver: solver.cpp
	g++ $(CFLAGS) -o $@ $<
