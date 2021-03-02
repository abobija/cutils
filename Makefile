CC=gcc

null:
	@:

test-custring: tests/custring.test.o custring.o
	$(CC) -o tests/custring.test tests/custring.test.o custring.o -I.

clean:
	find ./ -name "*.o" -type f -delete