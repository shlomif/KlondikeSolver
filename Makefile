all: KlondikeSolver

KlondikeSolver: solver.cpp
	g++ -g -o $@ $<
