// This code makes your Arduino act as a 3DConnexion SpaceMouse (32u4-based board required).
// To make this work you also need to set the USB Vendor ID and Product ID to values matching a real 3DConnexion device.
// You can do this by editing appropriate entries in the boards.txt file in your Arduino installation.
// Example values: vid=0x256f, pid=0xc631 (SpaceMouse Pro Wireless (cabled))
// Then install the 3DxWare software from 3DConnexion on your computer and
// you can use you Arduino device in software like Fusion 360 as it it were a real SpaceMouse.

#include "HID.h"

#define DEAD_THRESH 1 // Deazone for ignoring small movement

#define SPEED_DIVIDER_TX 2
#define SPEED_DIVIDER_TY 2
#define SPEED_DIVIDER_TZ 4
#define SPEED_DIVIDER_RX 3
#define SPEED_DIVIDER_RY 3
#define SPEED_DIVIDER_RZ 3

/*
The HID report descriptor starts with some generic desktop usage pages and collections.

Then it defines 3 reports with different report IDs:
- Report 1 (ID 0x01): Contains 3 16-bit absolute axis values for X, Y, Z
                      with logical range -500 to 500 and physical range -32768 to 32767.
- Report 2 (ID 0x02): Contains another 3 16-bit absolute axis values for RX, RY, RZ with the same ranges as report 1.
- Report 3 (ID 0x03): Contains 32 1-bit absolute button values with logical range 0 to 1.
*/

static const uint8_t _hidReportDescriptor[] PROGMEM = {
    0x05, 0x01, //  Usage Page (Generic Desktop)
    0x09, 0x08, //  0x08: Usage (Multi-Axis)
    0xa1, 0x01, //  Collection (Application)

    0xa1, 0x00,       // Collection (Physical)
    0x85, 0x01,       //  Report ID
    0x16, 0x00, 0x80, // logical minimum (-500)
    0x26, 0xff, 0x7f, // logical maximum (500)
    0x36, 0x00, 0x80, // Physical Minimum (-32768)
    0x46, 0xff, 0x7f, // Physical Maximum (32767)
    0x09, 0x30,       //    Usage (X)
    0x09, 0x31,       //    Usage (Y)
    0x09, 0x32,       //    Usage (Z)
    0x75, 0x10,       //    Report Size (16)
    0x95, 0x03,       //    Report Count (3)
    0x81, 0x02,       //    Input (variable,absolute)
    0xC0,             //  End Collection

    0xa1, 0x00,       // Collection (Physical)
    0x85, 0x02,       //  Report ID
    0x16, 0x00, 0x80, // logical minimum (-500)
    0x26, 0xff, 0x7f, // logical maximum (500)
    0x36, 0x00, 0x80, // Physical Minimum (-32768)
    0x46, 0xff, 0x7f, // Physical Maximum (32767)
    0x09, 0x33,       //    Usage (RX)
    0x09, 0x34,       //    Usage (RY)
    0x09, 0x35,       //    Usage (RZ)
    0x75, 0x10,       //    Report Size (16)
    0x95, 0x03,       //    Report Count (3)
    0x81, 0x02,       //    Input (variable,absolute)
    0xC0,             //  End Collection

    0xa1, 0x00, // Collection (Physical)
    0x85, 0x03, //  Report ID
    0x15, 0x00, //   Logical Minimum (0)
    0x25, 0x01, //    Logical Maximum (1)
    0x75, 0x01, //    Report Size (1)
    0x95, 32,   //    Report Count (24)
    0x05, 0x09, //    Usage Page (Button)
    0x19, 1,    //    Usage Minimum (Button #1)
    0x29, 32,   //    Usage Maximum (Button #24)
    0x81, 0x02, //    Input (variable,absolute)
    0xC0,       // End Collection

    0xC0 // End Collection
};

#define JOYSTICKS 4
#define X 0
#define Y 1
#define Z 2
#define T 0 // translation
#define R 1 // rotation

// ports of analog input for joysticks
int port[2][JOYSTICKS] = {
    {A6, A0, A2, A8},
    {A7, A1, A3, A9},
};

#define abs(x) ((x) < 0 ? (-x) : (x))

// initial sensor values
int origin[2][JOYSTICKS];

void setup()
{
    Serial.begin(9600);

    static HIDSubDescriptor node(_hidReportDescriptor, sizeof(_hidReportDescriptor));
    HID().AppendDescriptor(&node);

    for (int i = 0; i < JOYSTICKS; i++)
        for (int j = 0; j <= 1; j++)
            origin[j][i] = analogRead(port[j][i]);
}

void log(int16_t x, int16_t y, int16_t z, int16_t rx, int16_t ry, int16_t rz)
{
    Serial.print(x);
    Serial.print(", ");
    Serial.print(y);
    Serial.print(", ");
    Serial.print(z);
    Serial.print(", ");
    Serial.print(rx);
    Serial.print(", ");
    Serial.print(ry);
    Serial.print(", ");
    Serial.println(rz);
}

void send_command(int16_t x, int16_t y, int16_t z, int16_t rx, int16_t ry, int16_t rz)
{
    uint8_t trans[6] = {x & 0xFF, x >> 8, y & 0xFF, y >> 8, z & 0xFF, z >> 8};
    HID().SendReport(1, trans, 6);
    uint8_t rot[6] = {rx & 0xFF, rx >> 8, ry & 0xFF, ry >> 8, rz & 0xFF, rz >> 8};
    HID().SendReport(2, rot, 6);
}

void loop()
{
    int sv[2][JOYSTICKS]; // sensor value
    int mv[3][2];

    // read sensor value and subtract original position
    for (int i = 0; i < JOYSTICKS; i++)
        for (int j = 0; j <= 1; j++)
            sv[j][i] = analogRead(port[j][i]) - origin[j][i];

    mv[X][T] = (sv[Y][2] - sv[Y][0]) / SPEED_DIVIDER_TX;
    mv[Y][T] = (sv[Y][3] - sv[Y][1]) / SPEED_DIVIDER_TY;
    mv[Z][T] = -(sv[X][0] + sv[X][1] + sv[X][2] + sv[X][3]) / SPEED_DIVIDER_TZ;
    mv[X][R] = (sv[X][0] - sv[X][2]) / SPEED_DIVIDER_RX;
    mv[Y][R] = (sv[X][1] - sv[X][3]) / SPEED_DIVIDER_RY;
    mv[Z][R] = -(sv[Y][0] + sv[Y][1] + sv[Y][2] + sv[Y][3]) / SPEED_DIVIDER_RZ;

    bool moved = false;
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 2; j++)
            if (abs(mv[i][j]) > DEAD_THRESH)
                moved = true;
            else
                mv[i][j] = 0;

    if (moved)
    {
        log(sv[0][0], sv[0][1], sv[1][0], sv[1][1], sv[2][0], sv[2][1]);
        log(mv[X][T], mv[Y][T], mv[Z][T], mv[X][R], mv[Y][R], mv[Z][R]);
        send_command(mv[X][T], mv[Y][T], mv[Z][T], mv[X][R], mv[Y][R], mv[Z][R]);
    }
}
