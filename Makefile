cf: main.c
	gcc -o cf main.c -lncursesw

run: cf
	./cf
