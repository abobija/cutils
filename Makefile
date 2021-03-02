CC=gcc

null:
	@:

test-estr: tests/estr.test.o estr.o
	$(CC) -o tests/estr.test tests/estr.test.o estr.o -I. && tests/estr.test

test-cmder: tests/cmder.test.o cmder.o estr.o
	$(CC) -o tests/cmder.test tests/cmder.test.o cmder.o estr.o -I. && tests/cmder.test

clean:
	find ./ -type f \( -iname "*.o" -o -iname "*.test" \) -delete