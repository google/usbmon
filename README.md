USBTOOL
=======
A tool to enumerate, list and monitor USB devices connected to a Linux host.

The tool will print out a list of connected devices with USB path, vendor, serial and connection speed. Following that it will start monitoring for udev events such as connect or disconnect and print them with a timestamp. Uses `libudev`.

Requirement
-----------
* libudev

You can simply install it using fallowing command:
```
sudo apt-get install libudev-dev
```

Usage
-----

```
usbtool [-n] [-l]

  -n (optional) do not monitor and print events
  -l (optional) output the result into log.txt
```

License
-------
Apache 2.0

Disclaimer
----------
This is not an official Google product.
