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

ESTR_OBJS   = $(SRCDIR)/estr.o
XLIST_OBJS  = $(SRCDIR)/xlist.o
CMDER_OBJS  = $(ESTR_OBJS) $(XLIST_OBJS) $(SRCDIR)/cmder.o

.PHONY: all test clean

all: cutils estr cmder
	@:

$(SRCOBJ) $(TESTOBJ) $(TESTBIN):
	mkdir -p $@

$(SRCDIR)/%.o: $(SRCDIR)/%.c Makefile $(SRCOBJ)
	$(CC) -c -o $(OBJDIR)/$@ $<

$(TESTSDIR)/%.o: $(TESTSDIR)/%.c Makefile $(TESTOBJ) $(TESTBIN)
	$(CC) -c -o $(OBJDIR)/$@ $<

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

cutils.test: $(ESTR_OBJS) $(TESTSDIR)/cutils.test.o
	$(CC) -o $(TESTBIN)/$@ $(addprefix $(OBJDIR)/, $^)
	$(TESTBIN)/cutils.test && echo "cuilts tests passed"

estr.test: $(ESTR_OBJS) $(TESTSDIR)/estr.test.o
	$(CC) -o $(TESTBIN)/$@ $(addprefix $(OBJDIR)/, $^)
	$(TESTBIN)/estr.test && echo "estr tests passed"

xlist.test: $(XLIST_OBJS) $(TESTSDIR)/xlist.test.o
	$(CC) -o $(TESTBIN)/$@ $(addprefix $(OBJDIR)/, $^)
	$(TESTBIN)/xlist.test && echo "xlist tests passed"

cmder.test: $(CMDER_OBJS) $(TESTSDIR)/cmder.test.o
	$(CC) -o $(TESTBIN)/$@ $(addprefix $(OBJDIR)/, $^)
	$(TESTBIN)/cmder.test && echo "cmder tests passed"

clean:
	rm -rf ./$(OUTDIR)
	@echo "Project cleaned"