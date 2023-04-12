CC = clang-6106

HEADERS = $(wildcard *.h) $(wildcard utils/*.h) common/types.h
SOURCES = $(wildcard *.c) $(wildcard utils/*.c) common/types.c gen_eframes.c
OBJECTS = $(SOURCES:.c=.o)

BASE_CFLAGS =
BASE_LDFLAGS = -L/mit/6.172/arch/amd64_ubuntu1804/lib/clang-6106-lib

EXTRA_CFLAGS = -std=gnu11 -fopencilk
EXTRA_LDFLAGS = -fopencilk

# Determine the correct combination of flags and error nicely if it doesn't work.

ifeq ($(CILKSAN),1)
  EXTRA_CFLAGS += -fsanitize=cilk -DCILKSAN=1
  EXTRA_LDFLAGS += -fsanitize=cilk
endif

# We can only use AddressSanitizer if CilkSanitizer is not also in use.
ifeq ($(ASAN),1)
  ifeq ($(CILKSAN), 1)
    $(error cannot use asan and cilksan at same time)
  else
    EXTRA_CFLAGS += -fsanitize=address
    EXTRA_LDFLAGS += -fsanitize=address
  endif
endif

ifeq ($(UBSAN), 1)
	EXTRA_CFLAGS += -fsanitize=undefined
    EXTRA_LDFLAGS += -fsanitize=undefined
endif
ifeq ($(CILKSCALE),1)
	EXTRA_CFLAGS += -fcilktool=cilkscale
  	EXTRA_LDFLAGS += -fcilktool=cilkscale
endif

# This is for some extra scalability visualization that isn't currently implemented.

# ifeq ($(CILKBENCH), 1)
# 	EXTRA_CFLAGS += -fcilktool=cilkscale-benchmark
#  	EXTRA_LDFLAGS += -fcilktool=cilkscale-benchmark -lstdc++
# endif

# Determine which profile--debug or release--we should build against, and set
# CFLAGS appropriately.

ifeq ($(DEBUG),1)
  # We want debug mode.
  EXTRA_CFLAGS += -g -O0 -gdwarf-3
else
  # We want release mode.
  ifeq ($(CILKSAN),1)
    EXTRA_CFLAGS += -O0 -DNDEBUG
  else
    EXTRA_CFLAGS += -O3 -DNDEBUG
  endif
endif

export CC
export BASE_CFLAGS
export BASE_LDFLAGS
export EXTRA_CFLAGS
export EXTRA_LDFLAGS

# TODO try to use the Cilkscale visualization tool rather than the simple work-span measurements 
# that the old implementation provides.

all: libstudent libstaff ref-tester specvwr find-tier gen_eframes diff2gif convert_test

# How to compile a C file
%.o: %.c $(HEADERS)
	$(CC) $(BASE_CFLAGS) $(EXTRA_CFLAGS) -o $@ -c $<

.PHONY: format clean ref-tester specvwr find-tier libstaff libstudent convert_test gen_eframes diff2gif

format:
	clang-format -i -style=file *.h *.c **/*.h **/*.c

clean: 
	$(MAKE) -C libstaff clean
	$(MAKE) -C libstudent clean
	$(MAKE) -C ref-tester clean
	$(MAKE) -C specvwr clean
	$(MAKE) -C find-tier clean
	$(MAKE) -C convert_test clean
	$(MAKE) -C gen_eframes clean
	$(MAKE) -C diff2gif clean
	rm -f $(OBJECTS) $(TARGET)

ref-tester:
	$(MAKE) -C ref-tester

specvwr:
	$(MAKE) -C specvwr

find-tier:
	$(MAKE) -C find-tier

libstudent:
	$(MAKE) -C libstudent

libstaff:
	$(MAKE) -C libstaff

convert_test:
	$(MAKE) -C convert_test

gen_eframes:
	$(MAKE) -C gen_eframes

diff2gif:
	$(MAKE) -C diff2gif