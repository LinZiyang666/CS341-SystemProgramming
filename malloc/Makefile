# not using standard Makefile template because this makefile creates shared
# objects and weird stuff
CC = clang
WARNINGS = -Wall -Wextra -Werror -Wno-error=unused-parameter -Wmissing-declarations -Wmissing-variable-declarations
CFLAGS_COMMON = $(WARNINGS) -std=c99 -D_GNU_SOURCE -lm
CFLAGS_RELEASE_NO_LINK = $(WARNINGS) -std=c99 -D_GNU_SOURCE -O3
CFLAGS_RELEASE = $(CFLAGS_COMMON) -O3
CFLAGS_DEBUG = $(CFLAGS_COMMON) -O0 -g -DDEBUG

# the string in grep must appear in the hostname, otherwise the Makefile will
# not allow the assignment to compile
IS_VM=$(shell hostname | grep "cs341")
VM_OVERRIDE=$(shell echo $$HOSTNAME)
ifeq ($(IS_VM),)
ifneq ($(VM_OVERRIDE),cs241grader)
$(error This assignment must be compiled on the CS341 VMs)
endif
endif

TESTER_SRC = $(filter-out testers/tester-utils.c, $(wildcard testers/*.c))
TESTERS = $(patsubst %.c, %, $(TESTER_SRC))


all: alloc.so contest-alloc.so mreplace mcontest $(TESTERS:testers/%=testers_exe/%)

alloc.so: alloc.c
	$(CC) $^ $(CFLAGS_DEBUG) -o $@ -shared -fPIC -lm

mreplace: mcontest.c
	$(CC) $^ $(CFLAGS_RELEASE) -o $@ -ldl -lpthread

mcontest: mcontest.c contest.h
	$(CC) $< $(CFLAGS_RELEASE) -o $@ -ldl -lpthread -DCONTEST_MODE


# testers compiled in debug mode to prevent compiler from optimizing away the
# behavior we are trying to test
testers_exe/%: testers/%.c testers_exe/tester-utils.a
	@mkdir -p testers_exe/
	$(CC) $< testers_exe/tester-utils.a $(CFLAGS_DEBUG) -o $@

testers_exe/tester-utils.a: testers_exe/tester-utils.o
	@mkdir -p testers_exe/
	ar r testers_exe/tester-utils.a testers_exe/tester-utils.o

testers_exe/tester-utils.o: testers/tester-utils.c testers/tester-utils.h
	@mkdir -p testers_exe/
	$(CC) -c $< $(CFLAGS_RELEASE_NO_LINK) -o $@
	

.PHONY : clean
clean:
	-rm -rf *.o alloc.so mreplace mcontest testers_exe/
