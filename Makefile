CFLAGS = -Wall -O3 -o
LIBS = -ludev
PKG = usbmon_1.1-1
PKGDIR = $(PKG)/usr/local/bin

all: Makefile usbmon.c
	$(CC) $(CFLAGS) usbmon usbmon.c $(LIBS)
	strip usbmon

deb: usbmon
	mkdir -p $(PKGDIR)
	cp usbmon $(PKGDIR)
	dpkg-deb --build $(PKG)

clean:
	rm -f usbmon *.deb $(PKGDIR)/*
