/*
 * Copyright 2019 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <limits.h>
#include <sys/utsname.h>
#include <libudev.h>

// Local Function Prototypes
void logmsg(int state, char *msg, ...);
void logdev(struct udev_device *dev);
void usage();

struct collectd {
    double connected;
    uint32_t adds;
    uint32_t removes;
    double speeds[10];
};

char hostname[1024];
enum { INFO, WARNING, ERROR };

/*  Description:  Print help information to stdout
 */
void usage() {
  printf("usbmon [-h|--help][-n | -c][-t <secs>] \n\t-h|--help (optional) help\n" \
    "\t-n        (optional) do not monitor events\n" \
    "\t-c        (optional) collectd exec plugin mode\n" \
    "\t-t=SECS   (optional) set time interval for monitoring default:10\n");
}

/* Description:  Print the usb information to stdout
 * Input:		 dev   udev device object
 */
void logdev(struct udev_device *dev) {

	const char *usbpath, *vendor, *serial, *speed, *event;

	if (dev == NULL) {
		logmsg(ERROR, "logdev called with NULL device pointer");
		return;
	}

	usbpath = strstr(udev_device_get_devpath(dev), "usb");
    event = udev_device_get_action(dev);
    vendor = udev_device_get_property_value(dev, "ID_VENDOR_FROM_DATABASE");
    serial = udev_device_get_property_value(dev, "ID_SERIAL");
    speed = udev_device_get_sysattr_value(dev, "speed");
    logmsg(INFO, "%6s %s %s %s %s",
    		(event) ? event : "ENUMERATE",
    		(usbpath) ? usbpath : "N/A",
    		(vendor) ? vendor : "N/A",
            (serial) ? serial : "N/A",
            (speed) ? speed : "N/A"
    );
    fflush(stdout);
}
/* Description:  Print a log message to stdout
 * input:		 state  type of error message. INFO, WARNING, ERROR
 *               msg    message string
 */
void logmsg(int state, char *msg, ...) {
    va_list ap;
    time_t t;
    struct tm *l;
    char *err[] = { "", " WARNING:", " ERROR:" };

    time(&t);
    l = localtime(&t);

    if(state > 2)
        state = 2;
    if(state < 0)
        state = 0;
    fflush(stdout);

    printf("%04d/%02d/%02d %02d:%02d:%02d %s:%s ",
        l->tm_year + 1900, l->tm_mon + 1, l->tm_mday,
        l->tm_hour, l->tm_min, l->tm_sec,
        hostname, err[state]
    );
    va_start(ap, msg);
    vprintf(msg, ap);
    va_end(ap);
    if(state != INFO)
        printf(" [%s (%d)]", strerror(errno), errno);
    putchar('\n');
    fflush(stdout);
    if(state == ERROR)
        exit(1);
}
/* Description:  Print PUTVAL string for collectd
 * input:		 collectd   counters and speeds
 *               interval   collectd interval (seconds)
 */
void putval(struct collectd *cv, int interval) {
    //printf("PUTVAL %s/usbmon/usb_devices interval=%d N:%.0f:%u:%u\n", hostname, interval, cv->connected, cv->adds, cv->removes);
	printf("PUTVAL %s/usbmon/usb_devices N:%.0f:%u:%u\n", hostname, cv->connected, cv->adds, cv->removes);

	fflush(stdout);
}
/* Description:  Figure out the type of usb port from the speed string
 *
 * input:		 speed char  char string with the speed of the port
 * return:                   return the index into the gauge array   */
int usb_speed_index(const char *speed) {
    return 0;
}
/* Description:  Create the list of usb devices, and perform on going monitoring
 *
 * input:		 co          output collectd data
 *               nomon       Do not do monitoring of events
 */
 int usbmon(int co, int nomon, int interval) {

	struct udev *udev;
	struct udev_enumerate *enu;
	struct udev_monitor *mon;
	struct udev_device *dev;
	struct udev_list_entry *current_dev = NULL;
	struct timeval ti;
	struct collectd cv;
	struct utsname u;
	const char *path = NULL;
	const char *speed = NULL;
	double speeds[10];
	int fd, ret;
	fd_set fds;

	uname(&u);							// Get the posix system name
	strncpy(hostname, u.nodename, sizeof(hostname));
	udev = udev_new();
	if(!udev) {
		logmsg(ERROR, "udev_new() failed\n");
		return(-1);
	}
	enu = udev_enumerate_new(udev);		// Create list of usb devices
	if(!enu) {
		logmsg(ERROR, "udev_enumerate_new() failed\n");
		return(-1);
	}
	if (udev_enumerate_add_match_subsystem(enu, "usb") < 0) {
		logmsg(ERROR, "udev_enumerate_add_match_subsystem() failed.");
		return (-1);
	}
	if (udev_enumerate_scan_devices(enu) < 0) {
		logmsg(ERROR, "udev_enumerate_scan_devices() failed");
		return (-1);
	}

	//  Iterate through each entry till end of list (NULL).
	//  Either count the number of connected devices, or print out connected devices.
	memset(speeds,0,sizeof(speeds));
	current_dev = udev_enumerate_get_list_entry(enu);         // get first entry
	while(current_dev != NULL) {
		path = udev_list_entry_get_name(current_dev);
		dev = udev_device_new_from_syspath(udev, path);
		speed = udev_device_get_sysattr_value(dev, "speed");
		if(dev && strcmp(udev_device_get_devtype(dev), "usb_device") == 0) {
			if(co) {
				cv.speeds[usb_speed_index(speed)]++;
				cv.connected++;
			} else {
				logdev(dev);
			}
		}  // if
		udev_device_unref(dev);
		current_dev = udev_list_entry_get_next(current_dev);    // Get the next entry
	} // while
	udev_enumerate_unref(enu);									// clean up
	// If user did not want to monitor exit program
	if(nomon)
		return 0;
	// Make it pretty for serial output (non collectd)
	if(!co)
		logmsg(INFO, "--------- Begin USB Event Monitoring -----------");
	else
		putval(&cv, interval);									// Output the Initial values

	// Init mon and fd for monitoring
	mon = udev_monitor_new_from_netlink(udev, "udev");
	if(!mon)
		logmsg(ERROR, "udev_monitor_new_from_netlink() failed\n");
	udev_monitor_filter_add_match_subsystem_devtype(mon, "usb", NULL);
	udev_monitor_enable_receiving(mon);
	fd = udev_monitor_get_fd(mon);
	path = NULL;

	while(1) {
		FD_ZERO(&fds);
		FD_SET(fd, &fds);
		ti.tv_sec = interval;
		ti.tv_usec = 0;

		ret = select(fd + 1, &fds, NULL, NULL, &ti);
		if(ret < 0)
			break;
		// Process device event.  If usb, process add and remove
		if(FD_ISSET(fd, &fds)) {
			dev = udev_monitor_receive_device(mon);
			if(dev && strcmp(udev_device_get_devtype(dev), "usb_device") == 0) {
				if(co) {
					if(strcmp(udev_device_get_action(dev), "add")==0) {
						speed = udev_device_get_sysattr_value(dev, "speed");
						cv.speeds[usb_speed_index(speed)]++;
						cv.adds++;
						cv.connected++;
						putval(&cv, interval);
					} else if (strcmp(udev_device_get_action(dev), "remove")==0) {
						cv.removes++;
						cv.connected--;
						putval(&cv, interval);
					}
				} else {
					logdev(dev);
				}
				udev_device_unref(dev);
			}
		}

	}
	udev_unref(udev);
    return 0;
}
int main(int argc, char **argv) {

	int co = 0, nomon = 0, opt, interval = 10;
    while((opt = getopt(argc,argv,"ncht:")) != -1) {
    	switch (opt) {
            // Print out the usb device tree then exit
            case 'n':
              nomon = 1;
              break;
            // Run under COLLECTD.  Monitor USB device events and pass them via PUTVAR
            case 'c':
              co = 1;
              break;
            // Interval for polling for USB events.  Default: 10 Seconds.
            case 't':
              interval = atoi(optarg);
              break;

            default:
              usage();
              return 0;
    	}  // switch
	}  // while

    if(nomon && co) {
    	logmsg(ERROR, "-c means running under collectd, ignoring nomon");
    	usage();
    }

    usbmon(co,nomon,interval);

    return 0;
}
