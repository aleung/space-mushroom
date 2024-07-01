#include "Wire.h"
#include <MPU6050_light.h>
#include <SPI.h>
#include <stdint.h>

//*******Arduino pins********
#define SS 10
#define SCK 15
#define MOSI 16
#define MISO 14

MPU6050 mpu(Wire);
unsigned long timer = 0;
byte dataBuffer[8];

void setup() {
  Serial.begin(9600);

  SPI.begin();
  pinMode(SS, OUTPUT);
  pinMode(SCK, OUTPUT); 
  pinMode(MOSI, OUTPUT);
  pinMode(MISO, INPUT);

  Wire.begin();

  byte status = mpu.begin();
  Serial.print(F("MPU6050 status: "));
  Serial.println(status);
  while (status != 0) {
  }  // stop everything if could not connect to MPU6050

  Serial.println(F("Calculating offsets, do not move MPU6050"));
  delay(1000);
  mpu.calcOffsets();  // gyro and accelero
  Serial.println("Done!\n");
}

void loop() {
  mpu.update();

  if ((millis() - timer) > 10) {  // print data every 10ms
    // Serial.print("X : ");
    // Serial.print(mpu.getAngleX());
    // Serial.print("\tY : ");
    // Serial.print(mpu.getAngleY());
    // Serial.print("\tZ : ");
    // Serial.println(mpu.getAngleZ());
    timer = millis();
  }

  SPI.beginTransaction(SPISettings(160000, MSBFIRST, SPI_MODE1));  // 320000 is about max
  delay(5);
  digitalWrite(SS, LOW);
  delay(20);  // Short delay necessary here
  for (int i = 0; i < 8; i++) {
    dataBuffer[i] = SPI.transfer(255);  // Must transfer 1's, 0's don't work
  }
  digitalWrite(SS, HIGH);
  SPI.endTransaction();

  for (int i=0; i<8; i++) {
    Serial.print(dataBuffer[i], HEX);
    Serial.print(", ");
  }

  // Serial.print(dataBuffer[2] << 8 | dataBuffer[1], DEC);  // Alpha
  // Serial.print(" ,");
  // Serial.print(dataBuffer[4] << 8 | dataBuffer[3], DEC);  // Beta
  Serial.println("");
}
