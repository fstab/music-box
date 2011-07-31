#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/input.h>
#include <fcntl.h>

static const char *code2string(int code);

#define EV_PRESSED  1
#define EV_RELEASED 0
#define EV_REPEAT   2

static const char *value2string(int value) {
    switch ( value ) {
        case EV_PRESSED: return "EV_PRESSED";
        case EV_RELEASED: return "EV_RELEASED";
        case EV_REPEAT: return "EV_RELEASED";
        default: return "UNKNOWN";
    }
}

// http://www.linuxjournal.com/article/6429?page=0,0

static void print_driver_version(int fd) {
    int version;
    if (ioctl(fd, EVIOCGVERSION, &version)) {
        fprintf(stderr, "Failed to get driver version: %s\n", strerror(errno));
    }
    /* the EVIOCGVERSION ioctl() returns an int */
    /* so we unpack it and display it */
    printf("evdev driver version is %d.%d.%d\n", version >> 16, (version >> 8) & 0xff, version & 0xff);
}

static const char *bus_type(int bustype) {
    switch ( bustype ) {
        case BUS_PCI: return "PCI";
        case BUS_ISAPNP: return "ISAPNP";
        case BUS_USB: return "USB";
        case BUS_HIL: return "HIL";
        case BUS_BLUETOOTH: return "BLUETOOTH";
        case BUS_VIRTUAL: return "VIRTUAL";
        case BUS_ISA: return "ISA";
        case BUS_I8042: return "I8042";
        case BUS_XTKBD: return "XTKBD";
        case BUS_RS232: return "RS232";
        case BUS_GAMEPORT: return "GAMEPORT";
        case BUS_PARPORT: return "PARPORT";
        case BUS_AMIGA: return "AMIGA";
        case BUS_ADB: return "ADB";
        case BUS_I2C: return "I2C";
        case BUS_HOST: return "HOST";
        case BUS_GSC: return "GSC";
        case BUS_ATARI: return "ATARI";
        case BUS_SPI: return "SPI";
        default: return "UNKNOWN";
    }
}

static void print_device_info(int fd) {
    struct input_id device_info;
    if(ioctl(fd, EVIOCGID, &device_info)) {
        fprintf(stderr, "Failed to get device info: %s\n", strerror(errno));
        exit(-1);
    }
    printf("vendor %04hx product %04hx version %04hx is on a %s bus.\n", device_info.vendor, device_info.product, device_info.version, bus_type(device_info.bustype));
}


static void print_device_name(int fd) {
    char name[256]= "Unknown";
    if(ioctl(fd, EVIOCGNAME(sizeof(name)), name) < 0) {
        fprintf(stderr, "Failed to get device name: %s\n", strerror(errno));
        exit(-1);
    }
    printf("Device name: %s\n", name);
}


static void print_device_port(int fd) {
    char phys[256] = "Unknown";
    if(ioctl(fd, EVIOCGPHYS(sizeof(phys)), phys) < 0) {
        fprintf(stderr, "Failed to get device port: %s\n", strerror(errno));
    }
    printf("Device port: %s\n", phys);
}

static void print_event(int fd) {
    size_t rb, yalv;
    /* the events (up to 64 at once) */
    struct input_event ev[64];
    rb=read(fd,ev,sizeof(struct input_event)*64);
    if (rb < (int) sizeof(struct input_event)) {
        fprintf(stderr, "Failed to get read event: %s\n", strerror(errno));
        exit(-1);
    }
    for (yalv = 0; yalv < (int) (rb / sizeof(struct input_event)); yalv++) {
        if (EV_KEY == ev[yalv].type ) { // && ev[yalv].value == 1) {
            printf("%ld.%06ld ", ev[yalv].time.tv_sec, ev[yalv].time.tv_usec);
            printf("type %d code %d (%s) value %d (%s)\n", ev[yalv].type, ev[yalv].code, code2string(ev[yalv].code), ev[yalv].value, value2string(ev[yalv].value));
        }
    }
}

void print(const char *dev) {
    int fd = open(dev, O_RDONLY);
    if ( fd == -1 ) {
        fprintf(stderr, "Failed to open %s: %s\n", dev, strerror(errno));
        exit(-1);
    }
    printf("\n%s\n", dev);
    print_driver_version(fd);
    print_device_info(fd);
    print_device_name(fd);
    print_device_port(fd);
    close(fd);
}

int main(int argc, char **argv) {
    print("/dev/input/by-id/usb-040b_6543-event-kbd");
    print("/dev/input/event0");
    print("/dev/input/event1");
    print("/dev/input/event2");
    print("/dev/input/event3");
    print("/dev/input/event4");
    print("/dev/input/event5");
    print("/dev/input/event6");
    print("/dev/input/event7");
    print("/dev/input/event8");
    print("/dev/input/event9");

    while ( 1 ) {
        const char *dev="/dev/input/by-id/usb-040b_6543-event-kbd";
        int fd = open(dev, O_RDONLY);
        if ( fd == -1 ) {
            fprintf(stderr, "Failed to open %s: %s\n", dev, strerror(errno));
            exit(-1);
        }
        //printf("------------------------------------------------------\n");
        print_event(fd);
    }
}

//  cat /usr/include/linux/input.h | grep '#define KEY_' | perl -lne 's/#define KEY_([0-9A-Z_]*).*/case KEY_\1:\n    return "$1";/; print'

static const char *code2string(int code) {
switch ( code ) {
case KEY_RESERVED:
    return "RESERVED";
case KEY_ESC:
    return "ESC";
case KEY_1:
    return "1";
case KEY_2:
    return "2";
case KEY_3:
    return "3";
case KEY_4:
    return "4";
case KEY_5:
    return "5";
case KEY_6:
    return "6";
case KEY_7:
    return "7";
case KEY_8:
    return "8";
case KEY_9:
    return "9";
case KEY_0:
    return "0";
case KEY_MINUS:
    return "MINUS";
case KEY_EQUAL:
    return "EQUAL";
case KEY_BACKSPACE:
    return "BACKSPACE";
case KEY_TAB:
    return "TAB";
case KEY_Q:
    return "Q";
case KEY_W:
    return "W";
case KEY_E:
    return "E";
case KEY_R:
    return "R";
case KEY_T:
    return "T";
case KEY_Y:
    return "Y";
case KEY_U:
    return "U";
case KEY_I:
    return "I";
case KEY_O:
    return "O";
case KEY_P:
    return "P";
case KEY_LEFTBRACE:
    return "LEFTBRACE";
case KEY_RIGHTBRACE:
    return "RIGHTBRACE";
case KEY_ENTER:
    return "ENTER";
case KEY_LEFTCTRL:
    return "LEFTCTRL";
case KEY_A:
    return "A";
case KEY_S:
    return "S";
case KEY_D:
    return "D";
case KEY_F:
    return "F";
case KEY_G:
    return "G";
case KEY_H:
    return "H";
case KEY_J:
    return "J";
case KEY_K:
    return "K";
case KEY_L:
    return "L";
case KEY_SEMICOLON:
    return "SEMICOLON";
case KEY_APOSTROPHE:
    return "APOSTROPHE";
case KEY_GRAVE:
    return "GRAVE";
case KEY_LEFTSHIFT:
    return "LEFTSHIFT";
case KEY_BACKSLASH:
    return "BACKSLASH";
case KEY_Z:
    return "Z";
case KEY_X:
    return "X";
case KEY_C:
    return "C";
case KEY_V:
    return "V";
case KEY_B:
    return "B";
case KEY_N:
    return "N";
case KEY_M:
    return "M";
case KEY_COMMA:
    return "COMMA";
case KEY_DOT:
    return "DOT";
case KEY_SLASH:
    return "SLASH";
case KEY_RIGHTSHIFT:
    return "RIGHTSHIFT";
case KEY_KPASTERISK:
    return "KPASTERISK";
case KEY_LEFTALT:
    return "LEFTALT";
case KEY_SPACE:
    return "SPACE";
case KEY_CAPSLOCK:
    return "CAPSLOCK";
case KEY_F1:
    return "F1";
case KEY_F2:
    return "F2";
case KEY_F3:
    return "F3";
case KEY_F4:
    return "F4";
case KEY_F5:
    return "F5";
case KEY_F6:
    return "F6";
case KEY_F7:
    return "F7";
case KEY_F8:
    return "F8";
case KEY_F9:
    return "F9";
case KEY_F10:
    return "F10";
case KEY_NUMLOCK:
    return "NUMLOCK";
case KEY_SCROLLLOCK:
    return "SCROLLLOCK";
case KEY_KP7:
    return "KP7";
case KEY_KP8:
    return "KP8";
case KEY_KP9:
    return "KP9";
case KEY_KPMINUS:
    return "KPMINUS";
case KEY_KP4:
    return "KP4";
case KEY_KP5:
    return "KP5";
case KEY_KP6:
    return "KP6";
case KEY_KPPLUS:
    return "KPPLUS";
case KEY_KP1:
    return "KP1";
case KEY_KP2:
    return "KP2";
case KEY_KP3:
    return "KP3";
case KEY_KP0:
    return "KP0";
case KEY_KPDOT:
    return "KPDOT";
case KEY_ZENKAKUHANKAKU:
    return "ZENKAKUHANKAKU";
case KEY_102ND:
    return "102ND";
case KEY_F11:
    return "F11";
case KEY_F12:
    return "F12";
case KEY_RO:
    return "RO";
case KEY_KATAKANA:
    return "KATAKANA";
case KEY_HIRAGANA:
    return "HIRAGANA";
case KEY_HENKAN:
    return "HENKAN";
case KEY_KATAKANAHIRAGANA:
    return "KATAKANAHIRAGANA";
case KEY_MUHENKAN:
    return "MUHENKAN";
case KEY_KPJPCOMMA:
    return "KPJPCOMMA";
case KEY_KPENTER:
    return "KPENTER";
case KEY_RIGHTCTRL:
    return "RIGHTCTRL";
case KEY_KPSLASH:
    return "KPSLASH";
case KEY_SYSRQ:
    return "SYSRQ";
case KEY_RIGHTALT:
    return "RIGHTALT";
case KEY_LINEFEED:
    return "LINEFEED";
case KEY_HOME:
    return "HOME";
case KEY_UP:
    return "UP";
case KEY_PAGEUP:
    return "PAGEUP";
case KEY_LEFT:
    return "LEFT";
case KEY_RIGHT:
    return "RIGHT";
case KEY_END:
    return "END";
case KEY_DOWN:
    return "DOWN";
case KEY_PAGEDOWN:
    return "PAGEDOWN";
case KEY_INSERT:
    return "INSERT";
case KEY_DELETE:
    return "DELETE";
case KEY_MACRO:
    return "MACRO";
case KEY_MUTE:
    return "MUTE";
case KEY_VOLUMEDOWN:
    return "VOLUMEDOWN";
case KEY_VOLUMEUP:
    return "VOLUMEUP";
case KEY_POWER:
    return "POWER";
case KEY_KPEQUAL:
    return "KPEQUAL";
case KEY_KPPLUSMINUS:
    return "KPPLUSMINUS";
case KEY_PAUSE:
    return "PAUSE";
case KEY_SCALE:
    return "SCALE";
case KEY_KPCOMMA:
    return "KPCOMMA";
case KEY_HANGEUL:
    return "HANGEUL";
//case KEY_HANGUEL:
//    return "HANGUEL";
case KEY_HANJA:
    return "HANJA";
case KEY_YEN:
    return "YEN";
case KEY_LEFTMETA:
    return "LEFTMETA";
case KEY_RIGHTMETA:
    return "RIGHTMETA";
case KEY_COMPOSE:
    return "COMPOSE";
case KEY_STOP:
    return "STOP";
case KEY_AGAIN:
    return "AGAIN";
case KEY_PROPS:
    return "PROPS";
case KEY_UNDO:
    return "UNDO";
case KEY_FRONT:
    return "FRONT";
case KEY_COPY:
    return "COPY";
case KEY_OPEN:
    return "OPEN";
case KEY_PASTE:
    return "PASTE";
case KEY_FIND:
    return "FIND";
case KEY_CUT:
    return "CUT";
case KEY_HELP:
    return "HELP";
case KEY_MENU:
    return "MENU";
case KEY_CALC:
    return "CALC";
case KEY_SETUP:
    return "SETUP";
case KEY_SLEEP:
    return "SLEEP";
case KEY_WAKEUP:
    return "WAKEUP";
case KEY_FILE:
    return "FILE";
case KEY_SENDFILE:
    return "SENDFILE";
case KEY_DELETEFILE:
    return "DELETEFILE";
case KEY_XFER:
    return "XFER";
case KEY_PROG1:
    return "PROG1";
case KEY_PROG2:
    return "PROG2";
case KEY_WWW:
    return "WWW";
case KEY_MSDOS:
    return "MSDOS";
case KEY_COFFEE:
    return "COFFEE";
//case KEY_SCREENLOCK:
//    return "SCREENLOCK";
case KEY_DIRECTION:
    return "DIRECTION";
case KEY_CYCLEWINDOWS:
    return "CYCLEWINDOWS";
case KEY_MAIL:
    return "MAIL";
case KEY_BOOKMARKS:
    return "BOOKMARKS";
case KEY_COMPUTER:
    return "COMPUTER";
case KEY_BACK:
    return "BACK";
case KEY_FORWARD:
    return "FORWARD";
case KEY_CLOSECD:
    return "CLOSECD";
case KEY_EJECTCD:
    return "EJECTCD";
case KEY_EJECTCLOSECD:
    return "EJECTCLOSECD";
case KEY_NEXTSONG:
    return "NEXTSONG";
case KEY_PLAYPAUSE:
    return "PLAYPAUSE";
case KEY_PREVIOUSSONG:
    return "PREVIOUSSONG";
case KEY_STOPCD:
    return "STOPCD";
case KEY_RECORD:
    return "RECORD";
case KEY_REWIND:
    return "REWIND";
case KEY_PHONE:
    return "PHONE";
case KEY_ISO:
    return "ISO";
case KEY_CONFIG:
    return "CONFIG";
case KEY_HOMEPAGE:
    return "HOMEPAGE";
case KEY_REFRESH:
    return "REFRESH";
case KEY_EXIT:
    return "EXIT";
case KEY_MOVE:
    return "MOVE";
case KEY_EDIT:
    return "EDIT";
case KEY_SCROLLUP:
    return "SCROLLUP";
case KEY_SCROLLDOWN:
    return "SCROLLDOWN";
case KEY_KPLEFTPAREN:
    return "KPLEFTPAREN";
case KEY_KPRIGHTPAREN:
    return "KPRIGHTPAREN";
case KEY_NEW:
    return "NEW";
case KEY_REDO:
    return "REDO";
case KEY_F13:
    return "F13";
case KEY_F14:
    return "F14";
case KEY_F15:
    return "F15";
case KEY_F16:
    return "F16";
case KEY_F17:
    return "F17";
case KEY_F18:
    return "F18";
case KEY_F19:
    return "F19";
case KEY_F20:
    return "F20";
case KEY_F21:
    return "F21";
case KEY_F22:
    return "F22";
case KEY_F23:
    return "F23";
case KEY_F24:
    return "F24";
case KEY_PLAYCD:
    return "PLAYCD";
case KEY_PAUSECD:
    return "PAUSECD";
case KEY_PROG3:
    return "PROG3";
case KEY_PROG4:
    return "PROG4";
case KEY_DASHBOARD:
    return "DASHBOARD";
case KEY_SUSPEND:
    return "SUSPEND";
case KEY_CLOSE:
    return "CLOSE";
case KEY_PLAY:
    return "PLAY";
case KEY_FASTFORWARD:
    return "FASTFORWARD";
case KEY_BASSBOOST:
    return "BASSBOOST";
case KEY_PRINT:
    return "PRINT";
case KEY_HP:
    return "HP";
case KEY_CAMERA:
    return "CAMERA";
case KEY_SOUND:
    return "SOUND";
case KEY_QUESTION:
    return "QUESTION";
case KEY_EMAIL:
    return "EMAIL";
case KEY_CHAT:
    return "CHAT";
case KEY_SEARCH:
    return "SEARCH";
case KEY_CONNECT:
    return "CONNECT";
case KEY_FINANCE:
    return "FINANCE";
case KEY_SPORT:
    return "SPORT";
case KEY_SHOP:
    return "SHOP";
case KEY_ALTERASE:
    return "ALTERASE";
case KEY_CANCEL:
    return "CANCEL";
case KEY_BRIGHTNESSDOWN:
    return "BRIGHTNESSDOWN";
case KEY_BRIGHTNESSUP:
    return "BRIGHTNESSUP";
case KEY_MEDIA:
    return "MEDIA";
case KEY_SWITCHVIDEOMODE:
    return "SWITCHVIDEOMODE";
case KEY_KBDILLUMTOGGLE:
    return "KBDILLUMTOGGLE";
case KEY_KBDILLUMDOWN:
    return "KBDILLUMDOWN";
case KEY_KBDILLUMUP:
    return "KBDILLUMUP";
case KEY_SEND:
    return "SEND";
case KEY_REPLY:
    return "REPLY";
case KEY_FORWARDMAIL:
    return "FORWARDMAIL";
case KEY_SAVE:
    return "SAVE";
case KEY_DOCUMENTS:
    return "DOCUMENTS";
case KEY_BATTERY:
    return "BATTERY";
case KEY_BLUETOOTH:
    return "BLUETOOTH";
case KEY_WLAN:
    return "WLAN";
case KEY_UWB:
    return "UWB";
case KEY_UNKNOWN:
    return "UNKNOWN";
case KEY_VIDEO_NEXT:
    return "VIDEO_NEXT";
case KEY_VIDEO_PREV:
    return "VIDEO_PREV";
case KEY_BRIGHTNESS_CYCLE:
    return "BRIGHTNESS_CYCLE";
case KEY_BRIGHTNESS_ZERO:
    return "BRIGHTNESS_ZERO";
case KEY_DISPLAY_OFF:
    return "DISPLAY_OFF";
case KEY_WIMAX:
    return "WIMAX";
case KEY_RFKILL:
    return "RFKILL";
case KEY_OK:
    return "OK";
case KEY_SELECT:
    return "SELECT";
case KEY_GOTO:
    return "GOTO";
case KEY_CLEAR:
    return "CLEAR";
case KEY_POWER2:
    return "POWER2";
case KEY_OPTION:
    return "OPTION";
case KEY_INFO:
    return "INFO";
case KEY_TIME:
    return "TIME";
case KEY_VENDOR:
    return "VENDOR";
case KEY_ARCHIVE:
    return "ARCHIVE";
case KEY_PROGRAM:
    return "PROGRAM";
case KEY_CHANNEL:
    return "CHANNEL";
case KEY_FAVORITES:
    return "FAVORITES";
case KEY_EPG:
    return "EPG";
case KEY_PVR:
    return "PVR";
case KEY_MHP:
    return "MHP";
case KEY_LANGUAGE:
    return "LANGUAGE";
case KEY_TITLE:
    return "TITLE";
case KEY_SUBTITLE:
    return "SUBTITLE";
case KEY_ANGLE:
    return "ANGLE";
case KEY_ZOOM:
    return "ZOOM";
case KEY_MODE:
    return "MODE";
case KEY_KEYBOARD:
    return "KEYBOARD";
case KEY_SCREEN:
    return "SCREEN";
case KEY_PC:
    return "PC";
case KEY_TV:
    return "TV";
case KEY_TV2:
    return "TV2";
case KEY_VCR:
    return "VCR";
case KEY_VCR2:
    return "VCR2";
case KEY_SAT:
    return "SAT";
case KEY_SAT2:
    return "SAT2";
case KEY_CD:
    return "CD";
case KEY_TAPE:
    return "TAPE";
case KEY_RADIO:
    return "RADIO";
case KEY_TUNER:
    return "TUNER";
case KEY_PLAYER:
    return "PLAYER";
case KEY_TEXT:
    return "TEXT";
case KEY_DVD:
    return "DVD";
case KEY_AUX:
    return "AUX";
case KEY_MP3:
    return "MP3";
case KEY_AUDIO:
    return "AUDIO";
case KEY_VIDEO:
    return "VIDEO";
case KEY_DIRECTORY:
    return "DIRECTORY";
case KEY_LIST:
    return "LIST";
case KEY_MEMO:
    return "MEMO";
case KEY_CALENDAR:
    return "CALENDAR";
case KEY_RED:
    return "RED";
case KEY_GREEN:
    return "GREEN";
case KEY_YELLOW:
    return "YELLOW";
case KEY_BLUE:
    return "BLUE";
case KEY_CHANNELUP:
    return "CHANNELUP";
case KEY_CHANNELDOWN:
    return "CHANNELDOWN";
case KEY_FIRST:
    return "FIRST";
case KEY_LAST:
    return "LAST";
case KEY_AB:
    return "AB";
case KEY_NEXT:
    return "NEXT";
case KEY_RESTART:
    return "RESTART";
case KEY_SLOW:
    return "SLOW";
case KEY_SHUFFLE:
    return "SHUFFLE";
case KEY_BREAK:
    return "BREAK";
case KEY_PREVIOUS:
    return "PREVIOUS";
case KEY_DIGITS:
    return "DIGITS";
case KEY_TEEN:
    return "TEEN";
case KEY_TWEN:
    return "TWEN";
case KEY_VIDEOPHONE:
    return "VIDEOPHONE";
case KEY_GAMES:
    return "GAMES";
case KEY_ZOOMIN:
    return "ZOOMIN";
case KEY_ZOOMOUT:
    return "ZOOMOUT";
case KEY_ZOOMRESET:
    return "ZOOMRESET";
case KEY_WORDPROCESSOR:
    return "WORDPROCESSOR";
case KEY_EDITOR:
    return "EDITOR";
case KEY_SPREADSHEET:
    return "SPREADSHEET";
case KEY_GRAPHICSEDITOR:
    return "GRAPHICSEDITOR";
case KEY_PRESENTATION:
    return "PRESENTATION";
case KEY_DATABASE:
    return "DATABASE";
case KEY_NEWS:
    return "NEWS";
case KEY_VOICEMAIL:
    return "VOICEMAIL";
case KEY_ADDRESSBOOK:
    return "ADDRESSBOOK";
case KEY_MESSENGER:
    return "MESSENGER";
case KEY_DISPLAYTOGGLE:
    return "DISPLAYTOGGLE";
case KEY_SPELLCHECK:
    return "SPELLCHECK";
case KEY_LOGOFF:
    return "LOGOFF";
case KEY_DOLLAR:
    return "DOLLAR";
case KEY_EURO:
    return "EURO";
case KEY_FRAMEBACK:
    return "FRAMEBACK";
case KEY_FRAMEFORWARD:
    return "FRAMEFORWARD";
case KEY_CONTEXT_MENU:
    return "CONTEXT_MENU";
case KEY_MEDIA_REPEAT:
    return "MEDIA_REPEAT";
case KEY_10CHANNELSUP:
    return "10CHANNELSUP";
case KEY_10CHANNELSDOWN:
    return "10CHANNELSDOWN";
case KEY_DEL_EOL:
    return "DEL_EOL";
case KEY_DEL_EOS:
    return "DEL_EOS";
case KEY_INS_LINE:
    return "INS_LINE";
case KEY_DEL_LINE:
    return "DEL_LINE";
case KEY_FN:
    return "FN";
case KEY_FN_ESC:
    return "FN_ESC";
case KEY_FN_F1:
    return "FN_F1";
case KEY_FN_F2:
    return "FN_F2";
case KEY_FN_F3:
    return "FN_F3";
case KEY_FN_F4:
    return "FN_F4";
case KEY_FN_F5:
    return "FN_F5";
case KEY_FN_F6:
    return "FN_F6";
case KEY_FN_F7:
    return "FN_F7";
case KEY_FN_F8:
    return "FN_F8";
case KEY_FN_F9:
    return "FN_F9";
case KEY_FN_F10:
    return "FN_F10";
case KEY_FN_F11:
    return "FN_F11";
case KEY_FN_F12:
    return "FN_F12";
case KEY_FN_1:
    return "FN_1";
case KEY_FN_2:
    return "FN_2";
case KEY_FN_D:
    return "FN_D";
case KEY_FN_E:
    return "FN_E";
case KEY_FN_F:
    return "FN_F";
case KEY_FN_S:
    return "FN_S";
case KEY_FN_B:
    return "FN_B";
case KEY_BRL_DOT1:
    return "BRL_DOT1";
case KEY_BRL_DOT2:
    return "BRL_DOT2";
case KEY_BRL_DOT3:
    return "BRL_DOT3";
case KEY_BRL_DOT4:
    return "BRL_DOT4";
case KEY_BRL_DOT5:
    return "BRL_DOT5";
case KEY_BRL_DOT6:
    return "BRL_DOT6";
case KEY_BRL_DOT7:
    return "BRL_DOT7";
case KEY_BRL_DOT8:
    return "BRL_DOT8";
case KEY_BRL_DOT9:
    return "BRL_DOT9";
case KEY_BRL_DOT10:
    return "BRL_DOT10";
case KEY_NUMERIC_0:
    return "NUMERIC_0";
case KEY_NUMERIC_1:
    return "NUMERIC_1";
case KEY_NUMERIC_2:
    return "NUMERIC_2";
case KEY_NUMERIC_3:
    return "NUMERIC_3";
case KEY_NUMERIC_4:
    return "NUMERIC_4";
case KEY_NUMERIC_5:
    return "NUMERIC_5";
case KEY_NUMERIC_6:
    return "NUMERIC_6";
case KEY_NUMERIC_7:
    return "NUMERIC_7";
case KEY_NUMERIC_8:
    return "NUMERIC_8";
case KEY_NUMERIC_9:
    return "NUMERIC_9";
case KEY_NUMERIC_STAR:
    return "NUMERIC_STAR";
case KEY_NUMERIC_POUND:
    return "NUMERIC_POUND";
case KEY_CAMERA_FOCUS:
    return "CAMERA_FOCUS";
case KEY_WPS_BUTTON:
    return "WPS_BUTTON";
case KEY_TOUCHPAD_TOGGLE:
    return "TOUCHPAD_TOGGLE";
case KEY_TOUCHPAD_ON:
    return "TOUCHPAD_ON";
case KEY_TOUCHPAD_OFF:
    return "TOUCHPAD_OFF";
//case KEY_MIN_INTERESTING:
//    return "MIN_INTERESTING";
case KEY_MAX:
    return "MAX";
case KEY_CNT:
    return "CNT";
default:
    return "unknown";
}
}
