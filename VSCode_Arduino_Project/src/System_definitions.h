#ifndef DEFINITIONS_H
#define DEFINITIONS_H

// Debug Options (set to #ifdef)
//#define DEBUG_MOTOR  0
// #define DEBUG_HOME 0
//#define DEBUG_DIR   1
// #define DEBUG_COM0
 
 
// Define Pins (Set for Ardunio Mega2560)
#define JS_STOP_SWTICH  13

#define MTR_ENA_0 5
#define MTR_ENA_1 6
#define MTR_ENA_2 1

#define MTR_CS0 10 
#define MTR_CS1 11 
#define MTR_CS2 2 

#define MISOPIN  50 // UNUSED. noted for reference only. Due uses SPI header
#define MOSIPIN  51 // UNUSED. noted for reference only. Due uses SPI header
#define SCKPIN 52 // UNUSED. noted for reference only. Due uses SPI header
 
 
#define JS_XAXIS_INPUT A1
#define JS_YAXIS_INPUT A0
#define ADC_RESOLUTION_FLOAT 1024.0


// Additional constants
#define STAND_MTR_VELOCITY 	50000//100000
#define STAND_MTR3_VELOCITY 	400//100000
#define MTR_STATUS_SIZE		25
#define MOTOR_STEPS		    200 //Motor (1.8) 360/1.8


typedef enum {
    // Init State machine definitions
    INIT_BLE_STAT_FAILED      = 0,
    INIT_BNO80_STAT_FAILED      = 1,
    INIT_MOTOR1_STAT_FAILED      = 2,
    INIT_MOTOR2_STAT_FAILED      = 3,
    INIT_MOTOR3_STAT_FAILED      = 4,
} InitMotorState_t;


#endif



