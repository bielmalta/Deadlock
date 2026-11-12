banker: banker.o
	gcc banker.o -o banker

banker.o: banker.c
	gcc -Wall -Wextra -std=c11 -c banker.c -o banker.o

clean:
	rm -f banker.o banker