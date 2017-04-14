all: console coord

console: jms_console.o
	gcc -o jms_console jms_console.o -g

jms_console.o: jms_console.c
	gcc -g -c jms_console.c

coord: jms_coord.o
	gcc -o jms_coord jms_coord.c -g

jms_coord.o: jms_coord.c
	gcc -g -c jms_coord.c 

