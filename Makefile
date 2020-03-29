CFLAGS = -Wall -O3 -o
LIBS = -ludev

all: Makefile usbmon.c
	$(CC) $(CFLAGS) usbmon usbmon.c $(LIBS)
	strip usbmon

clean:
	rm -f usbmon
