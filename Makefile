CFLAGS = -Wall -O3 -o
LIBS = -ludev

all: Makefile usbtool.c
	$(CC) $(CFLAGS) usbtool usbtool.c $(LIBS)
	strip usbtool

clean:
	rm -f usbtool
