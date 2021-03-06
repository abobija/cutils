CCFLAGS     = -Iinclude
CCWARNINGS  = -Wall -Wextra# -Wpedantic
SRCDIR      = src
TESTSDIR    = tests
OUTDIR      = build
OBJDIR      = $(OUTDIR)/obj
BINDIR      = $(OUTDIR)/bin
SRCOBJ      = $(OBJDIR)/$(SRCDIR)
TESTOBJ     = $(OBJDIR)/$(TESTSDIR)
TESTBIN     = $(BINDIR)/$(TESTSDIR)
CC          = gcc $(CCFLAGS) $(CCWARNINGS)

ESTR_OBJS   = $(SRCOBJ)/estr.o
XLIST_OBJS  = $(SRCOBJ)/xlist.o
CMDER_OBJS  = $(ESTR_OBJS) $(XLIST_OBJS) $(SRCOBJ)/cmder.o

.PHONY: all test clean

all: cutils estr cmder
	@:

$(SRCOBJ) $(TESTOBJ) $(TESTBIN):
	mkdir -p $@

$(SRCOBJ)/%.o: $(SRCDIR)/%.c $(SRCOBJ) Makefile
	$(CC) -c -o $@ $<

$(TESTOBJ)/%.o: $(TESTSDIR)/%.c $(TESTOBJ) $(TESTBIN) Makefile
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

cutils.test: $(ESTR_OBJS) $(TESTOBJ)/cutils.test.o
	$(CC) -o $(TESTBIN)/$@ $^
	$(TESTBIN)/cutils.test && echo "cuilts tests passed"

estr.test: $(ESTR_OBJS) $(TESTOBJ)/estr.test.o
	$(CC) -o $(TESTBIN)/$@ $^
	$(TESTBIN)/estr.test && echo "estr tests passed"

xlist.test: $(XLIST_OBJS) $(TESTOBJ)/xlist.test.o
	$(CC) -o $(TESTBIN)/$@ $^
	$(TESTBIN)/xlist.test && echo "xlist tests passed"

cmder.test: $(CMDER_OBJS) $(TESTOBJ)/cmder.test.o
	$(CC) -o $(TESTBIN)/$@ $^
	$(TESTBIN)/cmder.test && echo "cmder tests passed"

clean:
	rm -rf ./$(OUTDIR)
	@echo "Project cleaned"