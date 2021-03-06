CCFLAGS = -I.
CCWARNINGS = -Wall -Wextra# -Wpedantic
CC = gcc $(CCFLAGS) $(CCWARNINGS)

ESTR_OBJS = estr.o
XLIST_OBJS = xlist.o
CMDER_OBJS = $(ESTR_OBJS) $(XLIST_OBJS) cmder.o

.PHONY: all test clean

all: cutils estr cmder
	@:

%.o: %.c Makefile
	$(CC) -c -o $@ $<

cutils:
	@:

estr: $(ESTR_OBJS)
	@:

xlist: $(XLIST_OBJS)
	@:

cmder: $(CMDER_OBJS)
	@:

test: cutils.test estr.test xlist.test cmder.test
	@echo "All test passed"

cutils.test: $(ESTR_OBJS) tests/cutils.test.o
	$(CC) -o tests/$@ $^
	tests/cutils.test && echo "cuilts tests passed"

estr.test: $(ESTR_OBJS) tests/estr.test.o
	$(CC) -o tests/$@ $^
	tests/estr.test && echo "estr tests passed"

xlist.test: $(XLIST_OBJS) tests/xlist.test.o
	$(CC) -o tests/$@ $^
	tests/xlist.test && echo "xlist tests passed"

cmder.test: $(CMDER_OBJS) tests/cmder.test.o
	$(CC) -o tests/$@ $^
	tests/cmder.test && echo "cmder tests passed"

clean:
	find ./ -type f \( -iname "*.o" -o -iname "*.test" \) -delete
	@echo "Project cleaned"