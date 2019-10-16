
TARGETS := multisched

MUL_OBJS := multisched.o

OBJS := $(MUL_OBJS)

CC := gcc

CFLAGS += -D_REENTRANT -D_LIBC_REENTRANT -D_THREAD_SAFE
CFLAGS += -Wall
CFLAGS += -Wunused
CFLAGS += -Wshadow
CFLAGS += -Wdeclaration-after-statement
CFLAGS += -Wdisabled-optimization
CFLAGS += -Wpointer-arith
CFLAGS += -Wredundant-decls
CFLAGS += -g -O2 

LDFLAGS +=

%.o: %.c 
	$(CC) -o $*.o $< -c $(CFLAGS)

.PHONY: all 

all: $(TARGETS)

multished: $(MUL_OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)



