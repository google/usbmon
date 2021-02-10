CFLAGS = -Wall -O3 -o
LIBS = -ludev
PKG = usbmon_1.1-1

all: Makefile usbmon.c
	$(CC) $(CFLAGS) usbmon usbmon.c $(LIBS)
	strip usbmon

deb: usbmon
	cp usbmon $(PKG)/usr/local/bin
	dpkg-deb --build $(PKG)

clean:
	rm -f usbmon *.deb $(PKG)/usr/local/bin/usbmon
