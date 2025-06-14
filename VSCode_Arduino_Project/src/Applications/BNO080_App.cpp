/***********************************************************************************************//**
 * @file       BNO080_App.cpp
 * @details    IMU application module. Services IMU initialization, data collection and filtering
 * @author		Miguel Silguero
 * @copyright  Copyright (c) 2024-2025, Horizonless Embedded Solutions LLC
 * @date       09.04.2024 (created)
 *
 **************************************************************************************************/

/***************************************************************************************************
 * INCLUDES
 **************************************************************************************************/
#include "BNO080_App.h"
#include <Wire.h>
#include "SparkFun_BNO080_Arduino_Library.h" /* Click here to get the library: http://librarymanager/All#SparkFun_BNO080 */
#include "System_definitions.h"
#include "Arduino.h"

/***************************************************************************************************
 * CONSTANTS AND DEFINITIONS
 **************************************************************************************************/
#define SAMPLES_TO_AVERAGE  (4)  /* Number of samples to use for averaging IMU data */
#define IMU_DATA_SET        (6)  /* Number of data points collected from the IMU */

/***************************************************************************************************
 * MODULE VARIABLES
 **************************************************************************************************/
BNO080 mBNO080IMU; /* IMU sensor object */
float IMU_Filtered_Data[IMU_DATA_SET][SAMPLES_TO_AVERAGE]; /* Buffer for storing past IMU data samples */
float IMU_Data[IMU_DATA_SET];         /* Stores the latest IMU data */
uint8_t DataSampleCount= 0;           /* Counter for averaging samples */
union floatUnion AveragedIMUdata[IMU_DATA_SET]; /* Averaged IMU data to be communicated to external interfaces */
uint32_t IMU_Comm_Errors = 0;
uint32_t IMU_NoDataCounter = 0;

/***************************************************************************************************
 * PRIVATE FUNCTION PROTOTYPES
 **************************************************************************************************/
void AverageIMUdata(void); /* Function prototype for filtering IMU data */
float ConverToPolarCoordinates(float neg_pos_degrees);

/***************************************************************************************************
 * FUNCTION DEFINITIONS
 **************************************************************************************************/

/***********************************************************************************************//**
 * @details     Initialize IMU pins and configuration. Enunciate when init
 **************************************************************************************************/
uint8_t BNO080_App :: Init(void)
{
  uint8_t initState;

  /* Configure I2C pins */
  pinMode(I2C_SDA, INPUT);
  pinMode(I2C_SCL, OUTPUT);
  digitalWrite(I2C_SCL, LOW); 

/* Attempt to initialize IMU */
  delay(350);
  Wire.begin();
  Wire.setClock(50000);
  //Wire.setClock(8000);
  
  if (mBNO080IMU.begin(BNO080_DEFAULT_ADDRESS, Wire) == false)
  {
    initState = 1; /* Initialization failed */
    Wire.end();
  }
  else
  {
    initState = 0; /* Initialization successful */
    Wire.setClock(400000); /* Increase I2C speed to 200kHz */
    //mBNO080IMU.enableRotationVector(IMU_DATA_ACQUSITION_PERIOD); /* Configure IMU to send updates every IMU_DATA_ACQUSITION_PERIOD */
    mBNO080IMU.enableGeomagneticRotationVector(IMU_DATA_ACQUSITION_PERIOD); /* Configure IMU to send updates every IMU_DATA_ACQUSITION_PERIOD */
    
    mBNO080IMU.enableAccelerometer(IMU_DATA_ACQUSITION_PERIOD); 

    for (int t = 0; t < 20; t++)
    {
      delay(IMU_DATA_ACQUSITION_PERIOD);
      Service_BNO080();
    }
    
  }

  return initState;
}

/***********************************************************************************************//**
 * @details     Service IMU. Collect roll, pitch, yaw, and acceleration for all axes.
 **************************************************************************************************/
void BNO080_App :: Service_BNO080(void)
{
  float roll, pitch, yaw = 0.0f;
  float AccelX,  AccelY = 0.0f, AccelZ = 0.0f;

  /* Check if new IMU data is available */
  if (mBNO080IMU.dataAvailable() == true)
  {
    IMU_NoDataCounter = 0;
    /* Read orientation data from IMU (roll, pitch, yaw in degrees) */
    roll = (mBNO080IMU.getRoll()) * 180.0 / PI;
    pitch = (mBNO080IMU.getPitch()) * 180.0 / PI;
    yaw = (mBNO080IMU.getYaw()) * 180.0 / PI;
        
    yaw = ConverToPolarCoordinates(yaw);

    /* Read acceleration data from IMU */
    AccelX = mBNO080IMU.getAccelX();
    AccelY = mBNO080IMU.getAccelY();
    AccelZ = mBNO080IMU.getAccelZ();

    if ( (isnan(yaw) == false) && (isnan(AccelZ) == false) ) /* Only process if collected data is a valid float */
    {
      /* Store orientation data, which is dependent on the orientation of the assembly installation */
      IMU_Data[0] = roll;
      IMU_Data[1] = pitch; 
      IMU_Data[2] = yaw;
      /* Store acceleration data */
      IMU_Data[3] = AccelX;
      IMU_Data[4] = AccelY; 
      IMU_Data[5] = AccelZ;

      AverageIMUdata(); /* Apply filtering to the collected IMU data */
    }
    else
    {
      IMU_Comm_Errors++;
    }
  }
  else
  {
    Wire.flush();
    IMU_NoDataCounter++;

    if (IMU_NoDataCounter >= 20) /* Missing 2 seconds of consecutive IMU data sets must mean a gross failure */
    {
      IMU_Comm_Errors++;
    }
  }
}

/***********************************************************************************************//**
 * @details     Average IMU data over multiple samples to reduce noise
 **************************************************************************************************/
void AverageIMUdata(void)
{
  uint8_t sampleNumber = 0;
  uint8_t elementNumber = 0;

  /* Store the latest IMU data sample into the buffer */
  for(sampleNumber = 0; sampleNumber < IMU_DATA_SET; sampleNumber++)
  {
    IMU_Filtered_Data[sampleNumber][DataSampleCount] = IMU_Data[sampleNumber];
  }

  DataSampleCount++;   /* Increment sample counter */

  if (DataSampleCount >= SAMPLES_TO_AVERAGE) /* If enough samples have been collected, compute the average */
  {
    DataSampleCount = 0; /* Reset counter */
    
    /* Compute the average of stored samples for each IMU data sample */
    for(sampleNumber = 0; sampleNumber < IMU_DATA_SET; sampleNumber++)
    {
      AveragedIMUdata[sampleNumber].f = 0.0f;
      
      /* Sum up stored samples */
      for(elementNumber = 0; elementNumber < SAMPLES_TO_AVERAGE; elementNumber++)
      {
        AveragedIMUdata[sampleNumber].f += IMU_Filtered_Data[sampleNumber][elementNumber];
      }  
      
      /* Compute the average */
      AveragedIMUdata[sampleNumber].f /= (float)SAMPLES_TO_AVERAGE;  
    }
  }
}


float ConverToPolarCoordinates(float neg_pos_degrees)
{
  if (neg_pos_degrees < 0.0)
    neg_pos_degrees = 180.0 + (neg_pos_degrees+180.0);
        
  return neg_pos_degrees;
}
