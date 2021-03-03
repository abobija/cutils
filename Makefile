CCFLAGS = -I.
CCWARNINGS = -Wall -Wextra
CC = gcc $(CCFLAGS) $(CCWARNINGS)

ESTR_OBJS = estr.o
CMDER_OBJS = $(ESTR_OBJS) cmder.o

.PHONY: all all.test clean

null:
	@:

%.o: %.c Makefile
	$(CC) -c -o $@ $<

all: estr cmder
	@:

estr: $(ESTR_OBJS)
	@:

cmder: $(CMDER_OBJS)
	@:

test: estr.test cmder.test
	@echo "All test passed"

estr.test: $(ESTR_OBJS) tests/estr.test.o
	$(CC) -o tests/$@ $^
	tests/estr.test && echo "estr tests passed"

cmder.test: $(CMDER_OBJS) tests/cmder.test.o
	$(CC) -o tests/$@ $^
	tests/cmder.test && echo "cmder tests passed"

clean:
	find ./ -type f \( -iname "*.o" -o -iname "*.test" \) -delete
	@echo "Project cleaned"