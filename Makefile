run	:main.o subs.o
	 g++ -o run main.o subs.o

main.o	:main.cpp global.h structs.h
	g++ -Wall -c main.cpp
subs.o	:subs.hpp global.h structs.h
	g++ -Wall -c subs.cpp
