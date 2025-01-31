#include "BNO080_App.h"
#include <Wire.h>
#include "SparkFun_BNO080_Arduino_Library.h" // Click here to get the library: http://librarymanager/All#SparkFun_BNO080
#include "System_definitions.h"

BNO080 mBNO080IMU;

// =============== Arduino Functions and Calls ===============

uint8_t BNO080_App :: Init()
{
  uint8_t initState;

  Wire.begin();

  //Are you using a ESP? Check this issue for more information: https://github.com/sparkfun/SparkFun_BNO080_Arduino_Library/issues/16
//  //=================================
//  delay(100); //  Wait for BNO to boot
//  // Start i2c and BNO080
//  Wire.flush();   // Reset I2C
//  IMU.begin(BNO080_DEFAULT_ADDRESS, Wire);
//  Wire.begin(4, 5);
//  Wire.setClockStretchLimit(4000);
//  //=================================

  if (mBNO080IMU.begin(0x4A) == false)
  {
    initState = 1;
    Serial.println(F("BNO080 not detected at default I2C address. Check your jumpers and the hookup guide."));
  }
  else
  {
    initState = 0;
    Wire.setClock(400000); //Increase I2C data rate to 400kHz
    mBNO080IMU.enableRotationVector(50); //Send data update every 50ms
    Serial.println(F("Rotation vector enabled"));
    Serial.println(F("Output in form roll, pitch, yaw"));
  }
  return initState;
}


void BNO080_App :: Service_BNO080()
{

  //Look for reports from the IMU
  if (mBNO080IMU.dataAvailable() == true)
  {
    float roll = (mBNO080IMU.getRoll()) * 180.0 / PI; // Convert roll to degrees
    float pitch = (mBNO080IMU.getPitch()) * 180.0 / PI; // Convert pitch to degrees
    float yaw = (mBNO080IMU.getYaw()) * 180.0 / PI; // Convert yaw / heading to degrees

    Serial.print(roll, 1);
    Serial.print(F(","));
    Serial.print(pitch, 1);
    Serial.print(F(","));
    Serial.print(yaw, 1);

    Serial.println();
  }

}