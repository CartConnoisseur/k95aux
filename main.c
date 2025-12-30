#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/select.h>
#include <string.h>
#include <linux/uinput.h>
#include <sys/time.h>
#include <stdbool.h>

#define fatal(str) perror(str); exit(1);

#define UINPUT_DEVICE_NAME "Auxillary Corsair K95 Key Input"

#define HIDRAW_DEVICE "/dev/hidraw0"
#define REPORT_ID 0x03

// unused, but doesnt affect output. "documentation," i guess
#define KEY_G1  0x00000001
#define KEY_G2  0x00000002
#define KEY_G3  0x00000004
#define KEY_G4  0x00000008
#define KEY_G5  0x00000010
#define KEY_G6  0x00000020

#define KEY_G7  0x00000040
#define KEY_G8  0x00000080
#define KEY_G9  0x00000100
#define KEY_G10 0x00000200
#define KEY_G11 0x00000400
#define KEY_G12 0x00000800

#define KEY_G13 0x00001000
#define KEY_G14 0x00002000
#define KEY_G15 0x00004000
#define KEY_G16 0x00008000
#define KEY_G17 0x00010000
#define KEY_G18 0x00020000

#define KEY_BRIGHTNESS 0x00040000
#define KEY_SUPER_LOCK 0x00080000

#define KEY_MR 0x00100000
#define KEY_M1 0x00200000
#define KEY_M2 0x00400000
#define KEY_M3 0x00800000

unsigned int old_state;
unsigned int state;

struct uinput_user_dev uinput_device;

unsigned short mapping[24] = {
    KEY_F1,  KEY_F2,  KEY_F3,
    KEY_F4,  KEY_F5,  KEY_F6,

    KEY_F7,  KEY_F8,  KEY_F9,
    KEY_F10, KEY_F11, KEY_F12,

    KEY_F13, KEY_F14, KEY_F15,
    KEY_F16, KEY_F17, KEY_F18,

    KEY_F19, KEY_F20,

    KEY_F21, KEY_F22, KEY_F23, KEY_F24
};

void read_report(int fd) {
    unsigned char buf[64];
    int n = read(fd, buf, sizeof(buf));

    if (n > 0) {
        unsigned char id = buf[0];
        if (id == REPORT_ID) {
            // horrible bit fuckery
            old_state = state;

            // G keys
            state = buf[16];
            state |= (buf[17] & 0x0f) << 8;
            state |= buf[18] << 10;

            // brightness / super lock
            state |= buf[9] << 11;
            state |= buf[13] << 19;

            // M keys
            state |= (buf[17] & 0xf0) << 16;
        }
    }
}

void _debug_print_state() {
    printf("\n--------\n");
    printf("%d %d %d\n", (state & KEY_G1)  > 0, (state & KEY_G2)  > 0, (state & KEY_G3)  > 0);
    printf("%d %d %d\n", (state & KEY_G4)  > 0, (state & KEY_G5)  > 0, (state & KEY_G6)  > 0);
    printf("\n");
    printf("%d %d %d\n", (state & KEY_G7)  > 0, (state & KEY_G8)  > 0, (state & KEY_G9)  > 0);
    printf("%d %d %d\n", (state & KEY_G10) > 0, (state & KEY_G11) > 0, (state & KEY_G12) > 0);
    printf("\n");
    printf("%d %d %d\n", (state & KEY_G13) > 0, (state & KEY_G14) > 0, (state & KEY_G15) > 0);
    printf("%d %d %d\n", (state & KEY_G16) > 0, (state & KEY_G17) > 0, (state & KEY_G18) > 0);
    printf("\n");

    printf("%d %d %d %d", (state & KEY_MR) > 0, (state & KEY_M1) > 0, (state & KEY_M2) > 0, (state & KEY_M3) > 0);
    printf(" | ");
    printf("%d %d", (state & KEY_BRIGHTNESS) > 0, (state & KEY_SUPER_LOCK) > 0);
    printf("\n");

    printf("\n%06x\n", state);
    printf("--------\n");
}

void create_uinput_device(int fd) {
    memset(&uinput_device, 0, sizeof(uinput_device));
    strncpy(uinput_device.name, UINPUT_DEVICE_NAME, UINPUT_MAX_NAME_SIZE);
    uinput_device.id.bustype = BUS_USB;
    uinput_device.id.vendor  = 0x1;
    uinput_device.id.product = 0x1;
    uinput_device.id.version = 1;

    if (ioctl(fd, UI_SET_EVBIT, EV_KEY) < 0) {
        fatal("ioctl UI_SET_EVBIT EV_KEY");
    }

    for (int i = 0; i < 256; i++) {
        if (ioctl(fd, UI_SET_KEYBIT, i) < 0) {
            fatal("ioctl UI_SET_KEYBIT");
        }
    }

    if (write(fd, &uinput_device, sizeof(uinput_device)) < 0) {
        fatal("failed to write uinput device");
    }

    if (ioctl(fd, UI_DEV_CREATE) < 0) {
        fatal("failed to create uinput device");
    }
}

void press_event(struct input_event *event, unsigned short code) {
    memset(event, 0, sizeof(event));
    gettimeofday(&(event->time), NULL);

    event->type = EV_KEY;
    event->code = code;
    event->value = 1;
}

void release_event(struct input_event *event, unsigned short code) {
    memset(event, 0, sizeof(event));
    gettimeofday(&(event->time), NULL);

    event->type = EV_KEY;
    event->code = code;
    event->value = 0;
}

void syn_event(struct input_event *event) {
    memset(event, 0, sizeof(event));
    gettimeofday(&(event->time), NULL);

    event->type = EV_SYN;
    event->code = SYN_REPORT;
    event->value = 0;
}

void write_state(int fd) {
    struct input_event event;

    for (int i = 0; i < 24; i++) {
        unsigned short code = mapping[i];

        bool pressed = ((state >> i) & 1) && !((old_state >> i) & 1);
        bool released = ((old_state >> i) & 1) && !((state >> i) & 1);

        if (pressed) {
            press_event(&event, code);
            if (write(fd, &event, sizeof(event)) < 0) {
                fatal("failed to write uinput press event");
            }
        } else if (released) {
            release_event(&event, code);
            if (write(fd, &event, sizeof(event)) < 0) {
                fatal("failed to write uinput release event");
            }
        }
    }

    syn_event(&event);
    if (write(fd, &event, sizeof(event)) < 0) {
        fatal("failed to write uinput syn event");
    }
}

void load_mapping() {
    const char *keys[24] = {
        "G1",  "G2",  "G3",
        "G4",  "G5",  "G6",

        "G7",  "G8",  "G9",
        "G10", "G11", "G12",

        "G13", "G14", "G15",
        "G16", "G17", "G18",

        "BRIGHTNESS", "SUPER_LOCK",

        "MR", "M1", "M2", "M3"
    };

    char path[256];

    const char* home = getenv("HOME");
    if (home == NULL) {
        printf("$HOME not set, using default mapping");
        return;
    };

    snprintf(path, sizeof(path), "%s/%s", home, ".config/k95aux/mapping");
    FILE *f = fopen(path, "r");
    if (f == NULL) {
        printf("mapping file does not exist, using default mapping");
        return;
    };

    char line[256];
    while (fgets(line, sizeof(line), f)) {
        char key[256];
        unsigned short code;
        if (sscanf(line, "%s %hu", key, &code) < 2) {
            continue;
        }

        printf("key: %s, code: %d\n", key, code);

        for (int i = 0; i < sizeof(keys); i++) {
            if (strcmp(key, keys[i]) == 0) {
                mapping[i] = code;
                break;
            }
        }

        // invalid key names just ignored
    }

    fclose(f);
}

int main(int argc, char **argv) {
    char *device = HIDRAW_DEVICE;
    if (argc > 1) {
        device = argv[1];
    }

    load_mapping();

    printf("opening %s\n", device);
    int hid_fd = open(device, O_RDONLY);
    if (hid_fd < 0) {
        fatal("failed to open hid device");
    }

    printf("opening /dev/uinput\n");
    int uinput_fd = open("/dev/uinput", O_WRONLY | O_NDELAY);
    if (uinput_fd < 0) {
        fatal("failed to open uinput device");
    }

    create_uinput_device(uinput_fd);

    while (1) {
        _debug_print_state();
        read_report(hid_fd);
        write_state(uinput_fd);
    }

    close(hid_fd);
}
