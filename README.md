# USBMON
A tool to enumerate, list and monitor USB devices connected to a Linux host.

* Print a list of connected devices with USB path, properties and speed
* Monitor udev events such as connect or disconnect and print them with a timestamp
* Collectd plugin mode
* JSON stream mode

## Requirements
requires `libudev-dev`

## Usage

### Flags

```
usbmon [-n] [-c] [-j]
  -n do not monitor events
  -c collectd exec plugin mode
  -j json stream mode
```

### Collectd plugin mode
Usbmon can operate as collectd exec plugin.

Add a new type to `/usr/share/collectd/types.db`:

```
usb_devices             connected:GAUGE:0:U, adds:COUNTER:U:U, removes:COUNTER:U:U
```

Create exec plugin config file, `/etc/collectd/collectd.conf.d/usbmon.conf`:

```
LoadPlugin Exec
<Plugin exec>
  Exec "nobody" "/var/lib/collectd/plugins/usbmon" "-c"
</Plugin>
```

Install usbmon in the plugins directory and restart collectd.

### JSON Stream mode
Usbmon can generate a streaming JSON output with per port events.
Streaming JSON is basically a never ending array. To use in Go:

```
type evt struct {
  Port   string
  Serial string
  Event  string
  Speed  float32
}
u := exec.Command("usbmon")
o, _ := u.StdoutPipe()
u.Start()
d := json.NewDecoder(o)
d.Token() // read opening [
for d.More() {
  var e evt
  d.Decode(&e)
  fmt.Printf("%+v\n", e)
}
```

## License
* Usbmon code is licensed under Apache 2.0
* Usbmon links with `libudev` which is GPL2.1

## Disclaimer
This is not an official Google product.

