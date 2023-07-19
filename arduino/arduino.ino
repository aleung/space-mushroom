// This code makes your Arduino act as a 3DConnexion SpaceMouse (32u4-based board required).
// To make this work you also need to set the USB Vendor ID and Product ID to values matching a real 3DConnexion device.
// You can do this by editing appropriate entries in the boards.txt file in your Arduino installation.
// Example values: vid=0x256f, pid=0xc631 (SpaceMouse Pro Wireless (cabled))
// Then install the 3DxWare software from 3DConnexion on your computer and
// you can use you Arduino device in software like Fusion 360 as it it were a real SpaceMouse.

#include "HID.h"

/*
The HID report descriptor starts with some generic desktop usage pages and collections.

Then it defines 3 reports with different report IDs:
- Report 1 (ID 0x01): Contains 3 16-bit absolute axis values for X, Y, Z
                      with logical range -500 to 500 and physical range -32768 to 32767.
- Report 2 (ID 0x02): Contains another 3 16-bit absolute axis values for RX, RY, RZ with the same ranges as report 1.
- Report 3 (ID 0x03): Contains 32 1-bit absolute button values with logical range 0 to 1.
*/

static const uint8_t _hidReportDescriptor[] PROGMEM = {
    0x05, 0x01,       //  Usage Page (Generic Desktop)
    0x09, 0x08,       //  0x08: Usage (Multi-Axis)
    0xa1, 0x01,       //  Collection (Application)

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

    0xC0        // End Collection
};

#define DOF 6

/// hardware
#define DEAD_THRESH 5  // Deazone for ignoring small movement
#define SPEED_PARAM 40 // larger is slower

// ports of analog input for joysticks
int port[DOF] = {A0, A2, A6, A1, A3, A7};

// conversion matrix from sensor input to rigid motion
int coeff[DOF][DOF] = {
    {0, 0, 0, 10, 10, -20},  // TX
    {0, 0, 0, 17, -17, 0},   // TY
    {10, 10, 10, 0, 0, 0},   // TZ
    {3, 3, -6, 0, 0, 0},     // RX
    {-6, 6, 0, 0, 0, 0},     // RY
    {0, 0, 0, 2, 2, 2},      // RZ
};

#define abs(x) ((x) < 0 ? (-x) : (x))

int origin[DOF]; // initial sensor values

void setup()
{
    Serial.begin(9600);

    static HIDSubDescriptor node(_hidReportDescriptor, sizeof(_hidReportDescriptor));
    HID().AppendDescriptor(&node);

    for (int i = 0; i < DOF; i++)
    {
        origin[i] = analogRead(port[i]);
    }
}

void log(int16_t x, int16_t y, int16_t z, int16_t rx, int16_t ry, int16_t rz) {
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

void send_command(int16_t rx, int16_t ry, int16_t rz, int16_t x, int16_t y, int16_t z)
{
    uint8_t trans[6] = {x & 0xFF, x >> 8, y & 0xFF, y >> 8, z & 0xFF, z >> 8};
    HID().SendReport(1, trans, 6);
    uint8_t rot[6] = {rx & 0xFF, rx >> 8, ry & 0xFF, ry >> 8, rz & 0xFF, rz >> 8};
    HID().SendReport(2, rot, 6);
}

void loop()
{
    int sv[DOF]; // sensor value
    int mv[DOF]; // motion vector

    // read sensor value and subtract original position
    for (int i = 0; i < DOF; i++)
    {
        sv[i] = analogRead(port[i]) - origin[i];
    }

    // log(sv[0],sv[1],sv[2],sv[3],sv[4],sv[5]);

    // calculate the motion of the "mushroom" knob
    for (int i = 0; i < DOF; i++)
    {
        mv[i] = 0;
        for (int j = 0; j < DOF; j++)
        {
            mv[i] += coeff[i][j] * sv[j];
        }

        mv[i] /= SPEED_PARAM;

        if ((mv[i] > -DEAD_THRESH) && (mv[i] < DEAD_THRESH))
        {
            mv[i] = 0;
        }

        else if (mv[i] > 500)
        {
            mv[i] = 500;
        }
        else if (mv[i] < -500)
        {
            mv[i] = -500;
        }
    }

    bool Movement = false;
    for (int i = 0; i < DOF; i++)
    {
        if (mv[i] != 0)
        {
            Movement = true;
        }
    }


    if (Movement = true)
    {
        log(mv[0], mv[1], mv[2], mv[3], mv[4], mv[5]);
        send_command(mv[3], mv[4], mv[5], mv[0], mv[1], mv[2]);
    }
}