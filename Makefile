all:
	$(CC) -Wall -O3 -o usbtool usbtool.c -ludev
	strip usbtool

clean:
	rm -f usbtool
