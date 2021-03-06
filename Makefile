CCFLAGS     = -Iinclude -MD -MP
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

.PRECIOUS: %/.sentinel
%/.sentinel:
	mkdir -p ${@D}
	touch $@

$(SRCOBJ)/%.o: $(SRCDIR)/%.c $(SRCOBJ)/.sentinel Makefile
	$(CC) -c -o $@ $<

$(TESTOBJ)/%.o: $(TESTSDIR)/%.c $(TESTOBJ)/.sentinel $(TESTBIN)/.sentinel Makefile
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

#-include $(ESTR_OBJS:.o=.d)
#-include $(XLIST_OBJS:.o=.d)
#-include $(CMDER_OBJS:.o=.d)