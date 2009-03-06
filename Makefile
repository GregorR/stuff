CC=gcc
ECFLAGS=
CFLAGS=-O2 -g $(ECFLAGS)

LD=$(CC)
ELDFLAGS=
LDFLAGS=$(CFLAGS) $(ELDFLAGS)

ALLINDENT_OBJS=allindent.o

all: allindent

allindent: $(ALLINDENT_OBJS)
	$(LD) $(LDFLAGS) $(ALLINDENT_OBJS) -o allindent

.SUFFIXES: .c .o

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f allindent $(ALLINDENT_OBJS)

deps:
	$(CC) -MM *.c > deps

include deps
