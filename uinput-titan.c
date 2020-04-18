#define _GNU_SOURCE
#include <linux/input.h>
#include <linux/uinput.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

static void insertEvent(int fd, unsigned short type, unsigned short code, int value) {
    struct input_event e;
    memset(&e, 0, sizeof(e));
    e.type = type;
    e.code = code;
    e.value = value;
    write(fd, &e, sizeof(e));
}

static int uinput_init() {
    int fd = open("/dev/uinput", O_RDWR);

    struct uinput_user_dev setup = {
        .id = {
            .bustype = BUS_VIRTUAL,
            .vendor = 0xdead,
            .product = 0xbeaf,
            .version = 3,
        },
        .name = "titan uinput",
        .ff_effects_max = 0,
    };
    write(fd, &setup, sizeof(setup));

    ioctl(fd, UI_SET_EVBIT, EV_REL);
    ioctl(fd, UI_SET_EVBIT, EV_KEY);
    ioctl(fd, UI_SET_RELBIT, REL_X);
    ioctl(fd, UI_SET_RELBIT, REL_Y);
    ioctl(fd, UI_SET_RELBIT, REL_HWHEEL);
    ioctl(fd, UI_SET_RELBIT, REL_WHEEL);
    ioctl(fd, UI_SET_KEYBIT, BTN_LEFT);
    ioctl(fd, UI_SET_KEYBIT, BTN_RIGHT);
    ioctl(fd, UI_SET_KEYBIT, BTN_WHEEL);
    ioctl(fd, UI_SET_PROPBIT, INPUT_PROP_POINTER);

    const char phys[] = "this/is/a/virtual/device/for/scrolling";
    ioctl(fd, UI_SET_PHYS, phys);
    ioctl(fd, UI_DEV_CREATE, NULL);
    return fd;
}

static int original_input_init() {
    char *path = NULL;
    for(int i=0; i<64;i++) {
        asprintf(&path, "/dev/input/event%d", i);
        int fd = open(path, O_RDWR);
        if(fd < 0) continue;
        char name[128];
        ioctl(fd, EVIOCGNAME(sizeof(name)), name);
        if(strcmp(name, "mtk-pad") == 0) {
            ioctl(fd, EVIOCGRAB, 1);
            return fd;
        }

        close(fd);
    }
    free(path);
    return -1;
}


static void ev_parse_rel(struct input_event e) {
    const char *relName = "Unknown";
    switch(e.code) {
        default:
            fprintf(stderr, "Unknown rel %d\n", e.code);
            break;
    }
    printf("Got rel event %s: %d\n", relName, e.value);
}

static void ev_parse_syn(struct input_event e) {
    const char *synName = "Unknown";
    switch(e.code) {
        case SYN_REPORT:
            synName = "Simple report\n";
            break;
        case SYN_MT_REPORT:
            synName = "Multi-touch report";
            break;
        default:
            fprintf(stderr, "Unknown syn %d\n", e.code);
            break;
    }
    printf("Got syn event %s: %d\n", synName, e.value);
}

static void ev_parse_key(struct input_event e) {
    const char *keyName = "Unknown";
    switch(e.code) {
        case BTN_TOOL_FINGER:
            keyName = "tool finger";
            break;
        case BTN_TOUCH:
            keyName = "touch";
            break;
        default:
            fprintf(stderr, "Unknown key %d\n", e.code);
            break;
    }
    printf("Got key event %s: %d\n", keyName, e.value);
}

static void ev_parse_abs(struct input_event e) {
    const char *absName = "Unknown";
    switch(e.code) {
        case ABS_MT_POSITION_X:
            absName = "MT X";
            break;
        case ABS_MT_POSITION_Y:
            absName = "MT Y";
            break;
        case ABS_MT_TOUCH_MAJOR:
            absName = "Major touch axis";
            break;
        case ABS_MT_TOUCH_MINOR:
            absName = "Minor touch axis";
            break;
        case ABS_X:
            absName = "X";
            break;
        case ABS_Y:
            absName = "Y";
            break;
        case ABS_MT_TRACKING_ID:
            absName = "MT tracking id";
            break;
        default:
            fprintf(stderr, "Unknown abs %x\n", e.code);
            break;
    }
    printf("Got abs event %s: %d\n", absName, e.value);
}

static int wasTouched, oldX, oldY;
//touchpanel resolution is 1440x720
static void decide(int ufd, int touched, int x, int y) {
    if(!touched) {
        wasTouched = 0;
        return;
    }
    if(!wasTouched && touched) {
        oldX = x;
        oldY = y;
        wasTouched = touched;
        return;
    }
    printf("%d, %d, %d, %d, %d\n", touched, x, y, y - oldY, x - oldX);
    if( (y - oldY) > 80) {
        insertEvent(ufd, EV_REL, REL_HWHEEL, 1);
        insertEvent(ufd, EV_SYN, SYN_REPORT, 0);
        oldY = y;
        oldX = x;
        return;
    }
    if( (y - oldY) < -80) {
        insertEvent(ufd, EV_REL, REL_HWHEEL, -1);
        insertEvent(ufd, EV_SYN, SYN_REPORT, 0);
        oldY = y;
        oldX = x;
        return;
    }
    if( (x - oldX) < -120) {
        insertEvent(ufd, EV_REL, REL_WHEEL, -1);
        insertEvent(ufd, EV_SYN, SYN_REPORT, 0);
        oldY = y;
        oldX = x;
        return;
    }
    if( (x - oldX) > 120) {
        insertEvent(ufd, EV_REL, REL_WHEEL, 1);
        insertEvent(ufd, EV_SYN, SYN_REPORT, 0);
        oldY = y;
        oldX = x;
        return;
    }
}

int main() {
    int ufd = uinput_init();
    int origfd = original_input_init();

    int currentlyTouched = 0;
    int currentX = -1;
    int currentY = -1;
    while(1) {
        struct input_event e;
        if(read(origfd, &e, sizeof(e)) != sizeof(e)) break;
        if(0) switch(e.type) {
            case EV_REL:
                ev_parse_rel(e);
                break;
            case EV_SYN:
                ev_parse_syn(e);
                break;
            case EV_KEY:
                ev_parse_key(e);
                break;
            case EV_ABS:
                ev_parse_abs(e);
                break;
            default:
                fprintf(stderr, "Unknown event type %d\n", e.type);
                break;
        };
        if(e.type == EV_KEY && e.code == BTN_TOUCH) {
            currentlyTouched = e.value;
        }
        if(e.type == EV_ABS && e.code == ABS_MT_POSITION_X) {
            currentX = e.value;
        }
        if(e.type == EV_ABS && e.code == ABS_MT_POSITION_Y) {
            currentY = e.value;
        }
        if(e.type == EV_SYN && e.code == SYN_REPORT) {
            decide(ufd, currentlyTouched, currentX, currentY);
        }
    }
}
