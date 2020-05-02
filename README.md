# USBMON
A tool to enumerate, list and monitor USB devices connected to a Linux host.

Print a list of connected devices with USB path, properties and speed.
Monitor udev events such as connect or disconnect and print them with a timestamp.

New: Experimental collectd plugin mode.

## Requirements
requires `libudev-dev`

## Usage

### Flags

```
usbmon [-n][-c]
  -n do not monitor events
  -c collectd exec plugin mode
  -t=SECS set time interval for monitoring default:10
```

### Experimental collectd plugin mode
Usbmon can operate as collectd exec plugin.

Add a new type to `/usr/share/collectd/types.db`:

```
usb_devices             connected:GAUGE:0:U, adds:COUNTER:U:U, removes:COUNTER:U:U
```

Create exec plugin config file, `/etc/collectd/collectd.conf.d/usbmon.conf`:

```
LoadPlugin Exec
<Plugin exec>
  Exec "nobody" "/var/lib/collectd/plugins/usbmon" "-c" "-t=<secs>"
</Plugin>
```

Install usbmon in the plugins directory and restart collectd.


## License
Apache 2.0

## Disclaimer
This is not an official Google product.
