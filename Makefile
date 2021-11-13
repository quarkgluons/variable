default: main.c variables.h
	gcc -Wall main.c -o main

runTest1: default
	./main test1.c
