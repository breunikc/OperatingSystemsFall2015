CC = gcc
CFLAGS=-g 
LIBS=-lpthread 
FLIBS=-lfl

HFILES = 

CCFILES = solnmyshell.c\
	lex.yy.c

OFILES = $(CCFILES:.c=.o)

all: link

.cc.o:	 $(CCFILES) $(HFILES)
	$(CC) $(CFLAGS) -c $*.cc

link:	$(OFILES)
	$(CC) $(CFLAGS) $(LIBS)  -o run $(OFILES) $(FLIBS)
clean:
	rm -f *.o *.*~ run
