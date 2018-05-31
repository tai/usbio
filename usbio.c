/*BINFMTC:-Wall -lusb
 * -*- mode: c; coding: utf-8 -*-
 *
 * usbio - Control Morphy-compatible USB-IO
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <usb.h>

#define USBIO_VID 0x0BFE
#define USBIO_PID 0x1003

#define USBIO_P0 0x01
#define USBIO_P1 0x02

struct usb_dev_handle *
usbio_open(struct usb_bus *bus, int num) {
    struct usb_device *dev;
    struct usb_dev_handle *udev;

    for (; bus; bus = bus->next) {
        for (dev = bus->devices; dev; dev = dev->next) {
            if ((dev->descriptor.idVendor  != USBIO_VID) ||
                (dev->descriptor.idProduct != USBIO_PID)) {
                continue;
            }
            if (num-- > 0) continue;
            
            if ((udev = usb_open(dev)) != NULL) {
                usb_detach_kernel_driver_np(udev, 0);
                usb_set_configuration(udev, dev->config->bConfigurationValue);
                usb_claim_interface(udev, dev->config->interface->altsetting->bInterfaceNumber);
            }
            return udev;
        }
    }
    return NULL;
}

int
usbio_close(struct usb_dev_handle *dev) {
    return usb_close(dev);
}

void
usage(void) {
    fprintf(stderr,
            "usbio - Controls PORT1 and PORT0 of Morphy-compatible USB-IO\n"
            "Usage: usbio [-help][-n devnum] [0xP1P0]\n"
            "Example:\n"
            "  # set and get port for second (\"-n 1\") USB-IO\n"
            "  $ usbio -n 1 0x1234\n"
            "  $ usbio -n 1\n"
            "  0x0234 # In order of P1P0, with P1 using only lower nibble.\n");
    exit(1);
}

int main(int argc, char **argv) {
    usb_dev_handle *udev;
    int devnum = 0;
    int newval = -1;
    char ctl[] = { 0x00, 0x00 };

    /* parse options */
    for (optind = 1; optind < argc; optind++) {
        if (strncmp(argv[optind], "-h", 2) == 0) {
            usage();
        }
        if (strncmp(argv[optind], "-n", 2) == 0) {
            optind++;
            devnum = atoi(argv[optind]);
            continue;
        }

        /* non-option arg */
        if (argv[optind][0] != '-') {
            newval = strtol(argv[optind], NULL, 16);
            continue;
        }
    }
    
    usb_init();
    usb_find_busses();
    usb_find_devices();
    
    if ((udev = usbio_open(usb_get_busses(), devnum)) == NULL) {
        fprintf(stderr, "Failed to open USBIO: %s\n", usb_strerror());
        exit(1);
    }

    if (newval >= 0) {
        ctl[0] = USBIO_P1;
        ctl[1] = (newval >> 8) & 0x0F;
        usb_control_msg(udev, USB_DT_HID, USB_REQ_SET_CONFIGURATION, 0, 0, ctl, sizeof(ctl), 1000);

        ctl[0] = USBIO_P0;
        ctl[1] = newval & 0xFF;
        usb_control_msg(udev, USB_DT_HID, USB_REQ_SET_CONFIGURATION, 0, 0, ctl, sizeof(ctl), 1000);
    }
    else {
        ctl[0] = USBIO_P1 + 0x02;
        usb_control_msg(udev, USB_DT_HID, USB_REQ_SET_CONFIGURATION, 0, 0, ctl, sizeof(ctl), 1000);
        usb_interrupt_read(udev, 1, ctl, sizeof(ctl), 1000);
        printf("0x%.2X", ctl[1] & 0x0F);
        ctl[0] = USBIO_P0 + 0x02;
        usb_control_msg(udev, USB_DT_HID, USB_REQ_SET_CONFIGURATION, 0, 0, ctl, sizeof(ctl), 1000);
        usb_interrupt_read(udev, 1, ctl, sizeof(ctl), 1000);
        printf("%.2X\n", ctl[1] & 0xFF);
    }
    
    usbio_close(udev);

    exit(0);
}
