/***********************************************************************************************//**
 * @file       System_definitions.h
 * @details
 * @author      Miguel Silguero
 * @copyright  Copyright (c) 2024-2025, Horizonless Embedded Solutions LLC
 * @date       11.13.2024 (created)
 *
 **************************************************************************************************/
#ifndef SYSTEM_DEFINITIONS_H
#define SYSTEM_DEFINITIONS_H

/***************************************************************************************************
 * CONSTANTS AND DEFINITIONS
 **************************************************************************************************/

 #define I2C_SDA             (20) 
 #define I2C_SCL             (21)

 #define BLUEFRUIT_SPI_RST   (16)
 #define JS_STOP_SWTICH      (13)
 #define SYS_LED             (12)
 #define MTR_CS1             (11) 
 #define MTR_CS0             (10)    
 #define BLUEFRUIT_SPI_CS    (8)
 #define BLUEFRUIT_SPI_IRQ   (7)
 #define MTR_ENA_1           (6)
 #define MTR_ENA_0           (5)
 #define BLUEFRUIT_INTERRUPT (4)
 #define MTR_ENA_2           (3)
 #define MTR_CS2             (2) 
 /* UART RX and TX pins 0 and 1 are reserved*/
  
 #define JS_XAXIS_INPUT A1
 #define JS_YAXIS_INPUT A0
 #define ADC_RESOLUTION_FLOAT 1024.0
 
 // Debug Options (set to #ifdef)
 //#define DEBUG_MOTOR  0
 // #define DEBUG_HOME 0
 //#define DEBUG_DIR   1
 // #define DEBUG_COM0
 // Additional constants
 #define STAND_MTR_VELOCITY 	  (50000)
 #define STAND_MTR3_VELOCITY    (250)
 #define MTR3_HOLD_POWER        (1)
 #define MTR3_RUN_POWER         (20)
 #define MTR3_ACCELERATION      (20)

 #define MTR_STATUS_SIZE	      (25)
 #define MOTOR_MICRO_STEPS      (200) //Motor is (1.8) 360/1.8

/***************************************************************************************************
 * TYPEDEFS
 **************************************************************************************************/
union floatUnion
{
  int i;
  float f;
}; /* This union to used to convert between the binary representations of integers and floats */

typedef enum 
{
    // Init State machine definitions
    INIT_BLE_STAT_FAILED      = 0,
    INIT_BNO80_STAT_FAILED      = 1,
    INIT_MOTOR1_STAT_FAILED      = 2,
    INIT_MOTOR2_STAT_FAILED      = 3,
    INIT_MOTOR3_STAT_FAILED      = 4,
} SystemInitState_e;

#if (DEBUG == 1) /* Define different values for runtime vs debug. Debug sessions causes status info to return unexpected values*/
  const unsigned int MOTOR_OK_STATUS[3] = {16777215, 16777183, 6220251};
#else
  const unsigned int MOTOR_OK_STATUS[3] = {12582996, 12582996, 12587028};
#endif

#endif



