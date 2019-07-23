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
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <limits.h>
#include <sys/utsname.h>

#include <libudev.h>

enum { INFO, WARNING, ERROR };
void logmsg(int state, char *msg, ...) {
    va_list ap;
    time_t t;
    struct tm *l;
    struct utsname u;
    char *err[] = { "", " WARNING:", " ERROR:" };
    
    time(&t);
    l = localtime(&t);
    uname(&u);
    
    if(state > 2) 
        state = 2;
    if(state < 0) 
        state = 0;
    fflush(stdout);
    
    printf("%04d/%02d/%02d %02d:%02d:%02d %s:%s ",
        l->tm_year + 1900, l->tm_mon + 1, l->tm_mday,
        l->tm_hour, l->tm_min, l->tm_sec,
        u.nodename, err[state]
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

int main(int argc, char **argv) {
    struct udev *udev;
    struct udev_enumerate *enu;
    struct udev_monitor *mon;
    struct udev_device *dev;
    struct udev_list_entry *lst;
    struct udev_list_entry *entr = NULL;
    const char *path, *usbpath, *vendor, *serial, *speed, *action;
    int fd,ret;
    fd_set fds;

    udev = udev_new();
    if(!udev)
        logmsg(ERROR, "udev_new() failed\n");

    enu = udev_enumerate_new(udev);
    if(!enu)
        logmsg(ERROR, "udev_enumerate_new() failed\n");
        
    udev_enumerate_add_match_subsystem(enu, "usb");
    udev_enumerate_scan_devices(enu);

    lst = udev_enumerate_get_list_entry(enu);
    udev_list_entry_foreach(entr, lst) {
        path = udev_list_entry_get_name(entr);
        dev = udev_device_new_from_syspath(udev, path);
        if(dev && strcmp(udev_device_get_devtype(dev), "usb_device") == 0) {
            usbpath = strstr(udev_device_get_devpath(dev), "usb");
            vendor = udev_device_get_property_value(dev, "ID_VENDOR_FROM_DATABASE");
            serial = udev_device_get_property_value(dev, "ID_SERIAL");
            speed = udev_device_get_sysattr_value(dev, "speed");
            printf("%s: %s %s %s\n",
                (usbpath) ? usbpath : "N/A",
                (vendor) ? vendor : "N/A",
                (serial) ? serial : "N/A",
                (speed) ? speed : "N/A"
            );
        }
        udev_device_unref(dev);
    }
    udev_enumerate_unref(enu);

    if(argc > 1 && strcmp(argv[1], "-n") == 0)
       return 0;

    logmsg(INFO, "--------- Begin USB Event Monitoring -----------");

    mon = udev_monitor_new_from_netlink(udev, "udev");
    if(!mon)
        logmsg(ERROR, "udev_monitor_new_from_netlink() failed\n");
    udev_monitor_filter_add_match_subsystem_devtype(mon, "usb", NULL);
    udev_monitor_enable_receiving(mon);

    fd = udev_monitor_get_fd(mon);

    while(1) {
        FD_ZERO(&fds);
        FD_SET(fd, &fds);

        ret = select(fd + 1, &fds, NULL, NULL, NULL);
        if(ret <= 0)
            break;

        if(FD_ISSET(fd, &fds)) {
            dev = udev_monitor_receive_device(mon);
            if(dev && strcmp(udev_device_get_devtype(dev), "usb_device") == 0) {
                usbpath = strstr(udev_device_get_devpath(dev), "usb");
                vendor = udev_device_get_property_value(dev, "ID_VENDOR_FROM_DATABASE");
                serial = udev_device_get_property_value(dev, "ID_SERIAL");
                speed = udev_device_get_sysattr_value(dev, "speed");
                action = udev_device_get_action(dev),
                logmsg(INFO, "%6s %s %s %s %s",
                    (action) ? action : "N/A",
                    (usbpath) ? usbpath : "N/A",
                    (vendor) ? vendor : "N/A",
                    (serial) ? serial : "N/A",
                    (speed) ? speed : "N/A"
                );
                udev_device_unref(dev);
            }
        }
    }
    udev_unref(udev);

    return 0;
}
