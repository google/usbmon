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
#include <getopt.h>

#define USAGE "usbmon [-n][-c] [-j]\n" \
    "  -n do not monitor events\n" \
    "  -c collectd exec plugin mode\n" \
    "  -j json stream output\n"

char hostname[1024];

struct collectd {
    double connected;
    uint32_t adds;
    uint32_t removes;
};

static struct option options[] = {
    {"help", no_argument, 0, 'h'},
    {"nomon", no_argument, 0, 'n'},
    {"collectd", no_argument, 0, 'c'},
    {"json", no_argument, 0, 'j'},
    {0, 0, 0, 0}
};

enum { INFO, WARNING, ERROR };
enum { NOLOG, LOG };
enum { NONE, TEXT, COLLD, JSON };

void logmsg(int state, char *msg, ...) {
    va_list ap;
    time_t t;
    struct tm *l;
    char *err[] = { "", " WARNING:", " ERROR:" };

    time(&t);
    l = localtime(&t);

    if(state > ERROR)
        state = ERROR;
    if(state < INFO)
        state = INFO;

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

void putval(struct udev_device *dev, struct collectd *cv) {
    const char *action;

    if((dev) && (action=udev_device_get_action(dev))) {
        if(strcmp(action, "add")==0) {
            cv->adds++;
            cv->connected++;
        } else if (strcmp(action, "remove")==0) {
            cv->removes++;
            cv->connected--;
        }
    }

    // TODO(tenox): add output throttling
    printf("PUTVAL %s/usbmon/usb_devices N:%.0f:%u:%u\n",
        hostname, cv->connected, cv->adds, cv->removes);
    fflush(stdout);
}

void devmsg(struct udev_device *dev, int log) {
    const char *usbpath, *vendor, *serial, *speed, *action;

    action = udev_device_get_action(dev);
    usbpath = udev_device_get_sysname(dev);
    vendor = udev_device_get_property_value(dev, "ID_VENDOR_FROM_DATABASE");
    serial = udev_device_get_property_value(dev, "ID_SERIAL");
    speed = udev_device_get_sysattr_value(dev, "speed");

    if(log) {
        logmsg(INFO, "%6s %s %s %s %s Mbps",
            (action) ? action : "N/A",
            (usbpath) ? usbpath : "N/A",
            (vendor) ? vendor : "N/A",
            (serial) ? serial : "N/A",
            (speed) ? speed : "N/A"
        );
        return;
    }

    printf("%s %s %s %s Mbps\n",
        (usbpath) ? usbpath : "N/A",
        (vendor) ? vendor : "N/A",
        (serial) ? serial : "N/A",
        (speed) ? speed : "N/A"
    );
}

void jsonstream(struct udev_device *dev) {
    const char *port, *serial, *action, *spd;
    float speed;

    action = udev_device_get_action(dev);
    port = udev_device_get_sysname(dev);
    serial = udev_device_get_sysattr_value(dev, "serial");
    spd = udev_device_get_sysattr_value(dev, "speed");
    speed = strtof((spd) ? spd : "0", NULL);

    if(!port)
        return;

    printf("{ "\
                "\"Port\": \"%s\", "\
                "\"Serial\": \"%s\", "\
                "\"Event\": \"%s\", "\
                "\"Speed\": %.1f "\
            "}, ",
        port,
        (serial) ? serial : "N/A",
        (action) ? action : "enum",
        speed
    );

    fflush(stdout);
}

void usbmon(int output) {
    struct udev *udev;
    struct udev_enumerate *enu;
    struct udev_monitor *mon;
    struct udev_device *dev;
    struct udev_list_entry *lst;
    struct udev_list_entry *entr = NULL;
    struct timeval ti;
    struct collectd cv;
    const char *path;
    int fd, ret;
    fd_set fds;

    memset(&cv, 0, sizeof(struct collectd));

    udev = udev_new();
    if(!udev)
        logmsg(ERROR, "udev_new() failed\n");

    enu = udev_enumerate_new(udev);
    if(!enu)
        logmsg(ERROR, "udev_enumerate_new() failed\n");

    udev_enumerate_add_match_subsystem(enu, "usb");
    udev_enumerate_scan_devices(enu);

    if(output==JSON)
        putchar('[');

    lst = udev_enumerate_get_list_entry(enu);
    udev_list_entry_foreach(entr, lst) {
        path = udev_list_entry_get_name(entr);
        dev = udev_device_new_from_syspath(udev, path);
        if(dev && strcmp(udev_device_get_devtype(dev), "usb_device") != 0) {
            udev_device_unref(dev);
            continue;
        }
        switch(output) {
            case COLLD:
                cv.connected++;
                break;
            case JSON:
                jsonstream(dev);
                break;
            default:
                devmsg(dev, NOLOG);
        }
        udev_device_unref(dev);
    }
    udev_enumerate_unref(enu);

    switch(output) {
        case NONE:
            return;
        case TEXT:
            logmsg(INFO, "--------- Begin USB Event Monitoring -----------");
            break;
        case COLLD:
            putval(NULL, &cv);
    }

    mon = udev_monitor_new_from_netlink(udev, "udev");
    if(!mon)
        logmsg(ERROR, "udev_monitor_new_from_netlink() failed\n");
    udev_monitor_filter_add_match_subsystem_devtype(mon, "usb", NULL);
    udev_monitor_enable_receiving(mon);

    fd = udev_monitor_get_fd(mon);

    while(1) {
        FD_ZERO(&fds);
        FD_SET(fd, &fds);
        ti.tv_sec = 10; // TODO(tenox): make this in to a flag
        ti.tv_usec = 0;

        ret = select(fd + 1, &fds, NULL, NULL, &ti);
        if(ret < 0)
            break;

        if(!FD_ISSET(fd, &fds))
            continue;

        dev = udev_monitor_receive_device(mon);
        if(!dev)
            continue;

        if(strcmp(udev_device_get_devtype(dev), "usb_device") != 0) {
            udev_device_unref(dev);
            continue;
        }

        switch(output) {
            case COLLD:
                putval(dev, &cv);
                break;
            case JSON:
                jsonstream(dev);
                break;
            default:
                devmsg(dev, LOG);
        }

        udev_device_unref(dev);
    }
    udev_unref(udev);

    return;
}

int main(int argc, char **argv) {
    struct utsname u;
    int c, optidx = 0;
    int o = 0;
    int output = TEXT;

    memset(&u, 0, sizeof(struct utsname));
    uname(&u);
    strncpy(hostname, u.nodename, sizeof(hostname));

    while(1) {
        c = getopt_long(argc, argv, "nchj", options, &optidx);
        if(c == -1)
            break;

        switch(c) {
            case 0:
            case 'h':
                puts(USAGE);
                return 0;
            case 'n':
                output = NONE;
                o++;
                break;
            case 'c':
                output = COLLD;
                o++;
                break;
            case 'j':
                output = JSON;
                o++;
                break;
        }
    }

    if(o>1) {
        fprintf(stderr, "Error: -c, -n, -j cannot be used together\n\n%s", USAGE);
        return 0;
    }

    usbmon(output);

    return 0;
}
