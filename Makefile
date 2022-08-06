CFLAGS = -O -Wall -pedantic
CC = gcc
MAIN = mush
OBJS = mush.o parseline.o

all: $(MAIN)

$(MAIN) : $(OBJS) 
	$(CC) $(CFLAGS) -o $(MAIN) $(OBJS)

mush.o: mush.c parseline.h
	$(CC) $(CFLAGS) -c mush.c

parseline.o: parseline.c parseline.h
	$(CC) $(CFLAGS) -c parseline.c

test:
	@./mush

clean:
	rm -f *.o $(MAIN) core