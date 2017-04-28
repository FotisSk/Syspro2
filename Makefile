all: console coord

console: jms_console.o
	gcc -o jms_console jms_console.o -g

jms_console.o: jms_console.c
	gcc -g -c jms_console.c

coord: jms_coord.o status.o statusall.o showactive.o showfinished.o showpools.o suspend.o resume.o
	gcc -o jms_coord jms_coord.o status.o statusall.o showactive.o showfinished.o showpools.o suspend.o resume.o -g

jms_coord.o: jms_coord.c
	gcc -g -c jms_coord.c 

status.o: status.c
	gcc -g -c status.c

statusall.o: statusall.c
	gcc -g -c statusall.c

showactive.o: showactive.c
	gcc -g -c showactive.c

showfinished.o: showfinished.c
	gcc -c -g showfinished.c

showpools.o: showpools.c
	gcc -c -g showpools.c

suspend.o: suspend.c
	gcc -c -g suspend.c

resume.o: resume.c
	gcc -c -g resume.c
