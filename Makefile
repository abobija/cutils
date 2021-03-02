CC=gcc

null:
	@:

test-estr: tests/estr.test.o estr.o
	$(CC) -o tests/estr.test tests/estr.test.o estr.o -I.

clean:
	find ./ -name "*.o" -type f -delete