USBMON
======
A tool to enumerate, list and monitor USB devices connected to a Linux host.

The tool will print out a list of connected devices with USB path, vendor, serial and connection speed. Following that it will start monitoring for udev events such as connect or disconnect and print them with a timestamp. Uses `libudev`.

Requirement
-----------
requires libudev-dev

Usage
-----

```
usbmon [-h|--help][-n]
  -h|--help (optional) help you with usbtool
  -n        (optional) do not monitor and print events
```

License
-------
Apache 2.0

Disclaimer
----------
This is not an official Google product.

