# Space Mushroom

6DOF mouse

## Design

MPU6050 - 3-Axis accelerometer and 3-Axis gyroscope

I2C, connects to pin 2 (SDA), 3 (SCL)

Arduino library: [MPU6050_light](https://github.com/rfetick/MPU6050_light)

MLX90333

SPI, connects to pin 10 (SS), 15 (SCLK), 14 and 16 (MOSI)

Pro Micro

## Firmware

### Find the Arduino Data Folder

On Linux, there is a file `~/.arduinoIDE/arduino-cli.yaml` which stores the basic setup of Arduino IDE (maybe also for Arduino CLI).

Check `directories.data`, in my case it points to `~/.arduino15`.

Navigate to `~/.arduino15/packages/SparkFun/hardware/avr/<version>/`, there is a file `boards.txt` which contains all board data.

### Create Customized Board

We will make the Arduino (pro micro board) act as a 3DConnexion SpaceMouse.
To make this work, set the USB Vendor ID and Product ID to values matching a real 3DConnexion device.
For example, use values: vid=0x256f, pid=0xc631 to act as SpaceMouse Pro Wireless (cabled).

Duplicate the `promicro` section in `boards.txt` and change the prefix to `spacemushroom`.

Modify below properties and keep others as is:

``` ini
spacemushroom.name=Space Mushroom
spacemushroom.build.usb_manufacturer="3Dconnexion"
spacemushroom.build.usb_product="SpaceMouse Pro Wireless (cabled)"
spacemushroom.build.vid=0x256f

spacemushroom.menu.cpu.16MHzatmega32U4.build.pid.0=0xc631
spacemushroom.menu.cpu.16MHzatmega32U4.build.pid.1=0xc631
spacemushroom.menu.cpu.16MHzatmega32U4.build.pid=0xc631
```

Save the file and restart Arduino IDE. Pick the board "Space mushroom".

### Compile and Upload

Connect the Arduino board to PC with USB. In Arduino IDE, compile and upload the program to the board.

## Driver

Follow instruction on https://github.com/DD1984/SpaceMouse_Fusion360_Wine
