cf: main.c
	gcc -lncursesw -o cf main.c

run: cf
	./cf
