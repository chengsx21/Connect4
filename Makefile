.PHONY: main

all: main

main:
	g++ -O2 main.cpp Judge.cpp Strategy.cpp -o main
