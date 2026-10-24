// vpad: a virtual Xbox 360 gamepad over /dev/uinput, for driving the game on an emulator (root).
// Reads one command per line from stdin: A B X Y LB RB BACK START GUIDE LS RS UP DOWN LEFT RIGHT
// (a 300 ms press: the emulator renders slowly enough to miss a shorter one), "LT"/"RT" (a trigger pull), "HOLD <LX|LY|RX|RY> <value> <ms>" (a stick held
// off-centre, -32768..32767, then released), "SLEEP <ms>", or "QUIT".
//
//   $NDK/.../bin/aarch64-linux-android33-clang -O2 -o vpad vpad.c
//   adb push vpad /data/local/tmp/ && adb shell chmod 755 /data/local/tmp/vpad
//   adb shell 'cd /data/local/tmp; mkfifo pad.fifo; (while true; do cat pad.fifo; done) | ./vpad'
//   adb shell 'echo A > /data/local/tmp/pad.fifo'      # one command per write
//
// Start it before the game so SDL lists it; the shim prefers it over the emulator's "qwerty2".
#include <fcntl.h>
#include <linux/uinput.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static int fd;

static void emit(int type, int code, int value)
{
    struct input_event ev;
    memset(&ev, 0, sizeof ev);
    ev.type = type;
    ev.code = code;
    ev.value = value;
    write(fd, &ev, sizeof ev);
}
static void sync_(void) { emit(EV_SYN, SYN_REPORT, 0); }

static void press(int code)
{
    emit(EV_KEY, code, 1); sync_();
    usleep(300000);
    emit(EV_KEY, code, 0); sync_();
}
static void axis(int code, int value)
{
    emit(EV_ABS, code, value); sync_();
    usleep(300000);
    emit(EV_ABS, code, 0); sync_();
}

int main(void)
{
    fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (fd < 0) { perror("open /dev/uinput"); return 1; }

    ioctl(fd, UI_SET_EVBIT, EV_KEY);
    int keys[] = { BTN_A, BTN_B, BTN_X, BTN_Y, BTN_TL, BTN_TR, BTN_SELECT, BTN_START, BTN_MODE,
                   BTN_THUMBL, BTN_THUMBR };
    for (unsigned i = 0; i < sizeof keys / sizeof *keys; i++) ioctl(fd, UI_SET_KEYBIT, keys[i]);

    ioctl(fd, UI_SET_EVBIT, EV_ABS);
    struct uinput_user_dev dev;
    memset(&dev, 0, sizeof dev);
    snprintf(dev.name, UINPUT_MAX_NAME_SIZE, "Microsoft X-Box 360 pad");
    dev.id.bustype = BUS_USB;
    dev.id.vendor = 0x045e;
    dev.id.product = 0x028e;
    dev.id.version = 0x0114;
    int sticks[] = { ABS_X, ABS_Y, ABS_RX, ABS_RY };
    for (unsigned i = 0; i < 4; i++) {
        ioctl(fd, UI_SET_ABSBIT, sticks[i]);
        dev.absmin[sticks[i]] = -32768; dev.absmax[sticks[i]] = 32767;
        dev.absflat[sticks[i]] = 128; dev.absfuzz[sticks[i]] = 16;
    }
    int triggers[] = { ABS_Z, ABS_RZ };
    for (unsigned i = 0; i < 2; i++) {
        ioctl(fd, UI_SET_ABSBIT, triggers[i]);
        dev.absmin[triggers[i]] = 0; dev.absmax[triggers[i]] = 255;
    }
    int hats[] = { ABS_HAT0X, ABS_HAT0Y };
    for (unsigned i = 0; i < 2; i++) {
        ioctl(fd, UI_SET_ABSBIT, hats[i]);
        dev.absmin[hats[i]] = -1; dev.absmax[hats[i]] = 1;
    }
    write(fd, &dev, sizeof dev);
    if (ioctl(fd, UI_DEV_CREATE) < 0) { perror("UI_DEV_CREATE"); return 1; }
    fprintf(stderr, "vpad: ready\n");

    char line[64];
    while (fgets(line, sizeof line, stdin)) {
        line[strcspn(line, "\r\n")] = 0;
        int ms;
        if (!strcmp(line, "A")) press(BTN_A);
        else if (!strcmp(line, "B")) press(BTN_B);
        else if (!strcmp(line, "X")) press(BTN_X);
        else if (!strcmp(line, "Y")) press(BTN_Y);
        else if (!strcmp(line, "LB")) press(BTN_TL);
        else if (!strcmp(line, "RB")) press(BTN_TR);
        else if (!strcmp(line, "BACK")) press(BTN_SELECT);
        else if (!strcmp(line, "START")) press(BTN_START);
        else if (!strcmp(line, "GUIDE")) press(BTN_MODE);
        else if (!strcmp(line, "LS")) press(BTN_THUMBL);
        else if (!strcmp(line, "RS")) press(BTN_THUMBR);
        else if (!strcmp(line, "UP")) axis(ABS_HAT0Y, -1);
        else if (!strcmp(line, "DOWN")) axis(ABS_HAT0Y, 1);
        else if (!strcmp(line, "LEFT")) axis(ABS_HAT0X, -1);
        else if (!strcmp(line, "RIGHT")) axis(ABS_HAT0X, 1);
        else if (!strcmp(line, "LT")) axis(ABS_Z, 255);
        else if (!strcmp(line, "RT")) axis(ABS_RZ, 255);
        else if (sscanf(line, "SLEEP %d", &ms) == 1) usleep(ms * 1000);
        else if (!strncmp(line, "HOLD ", 5)) {
            char name[8]; int value;
            if (sscanf(line + 5, "%7s %d %d", name, &value, &ms) == 3) {
                int code = !strcmp(name, "LX") ? ABS_X : !strcmp(name, "LY") ? ABS_Y
                         : !strcmp(name, "RX") ? ABS_RX : !strcmp(name, "RY") ? ABS_RY : -1;
                if (code >= 0) {
                    emit(EV_ABS, code, value); sync_();
                    usleep(ms * 1000);
                    emit(EV_ABS, code, 0); sync_();
                }
            }
        }
        else if (!strcmp(line, "QUIT")) break;
        else if (line[0]) fprintf(stderr, "vpad: unknown '%s'\n", line);
    }
    ioctl(fd, UI_DEV_DESTROY);
    close(fd);
    return 0;
}
