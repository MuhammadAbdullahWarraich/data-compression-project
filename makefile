all: main.o ini.o
	gcc main.o ini.o -o main
	rm *.o
main.o: main.c
	gcc -c main.c
ini.o: ./inih/ini.c
	gcc -c inih/ini.c
