/***********************************************************************************************//**
 * @file       LED_App.cpp
 * @details    IMU application module. Services IMU initialization, data collection, and filtering
 * @author		Miguel Silguero
 * @copyright  Copyright (c) 2024-2025, Horizonless Embedded Solutions LLC
 * @date       03.09.2025 (created)
 *
 **************************************************************************************************/

/***************************************************************************************************
 * INCLUDES
 **************************************************************************************************/
#include "LED_App.h"
#include "System_definitions.h"
#include <Arduino.h>

/***************************************************************************************************
 * CONSTANTS AND DEFINITIONS
 **************************************************************************************************/
#define LED_CODE_SIZE 20 /*Defines the size of each LED code pattern.*/
/* Create a two-dimensional array storing predefined LED blinking patterns*/
uint8_t LED_CODES[8][LED_CODE_SIZE] = {{1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0},
                                        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
                                        {1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
                                        {1,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
                                        {1,0,1,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0},
                                        {1,0,1,0,1,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0},
                                        {1,0,1,0,1,0,1,0,1,0,1,0,0,0,0,0,0,0,0,0},
                                        {1,0,1,0,1,0,1,0,1,0,1,0,1,0,0,0,0,0,0,0},
};
/***************************************************************************************************
 * MODULE VARIABLES
 **************************************************************************************************/
uint8_t LedCodeIndex = 0; /*Tracks the current index within the LED code pattern*/
uint8_t BitIndex = 0;     /*Tracks the current bit being processed in the SetLedCode value*/
uint8_t LedCode = 0;      /*Stores the current LED code being displayed*/
uint8_t SetLedCode = 0;   /*Stores the LED code set by the user*/

/***************************************************************************************************
 * FUNCTION DEFINITIONS
 **************************************************************************************************/

/***********************************************************************************************//**
 * @details     Initialize IMU pins and configuration. Sets up the LED output pin.
 **************************************************************************************************/
void LED_App :: Init(void)
{
    pinMode(SYS_LED, OUTPUT);
}

/***********************************************************************************************//**
 * @details     Service LED. Controls LED blinking pattern based on the SetLedCode value.
 *              The LED blinks according to the predefined LED_CODES array.
 *              For example, an LedCode of 0b011 will shown bit 0 and bit 1. Note
 *              that bit 0 will show as a long on and long short message
 **************************************************************************************************/
void LED_App :: Service_LED(void)
{
    bool breakForLoop = false;

    if (LED_CODES[LedCode][LedCodeIndex] == 1) /* Check if LED pattern bit is set */
    {
        digitalWrite(SYS_LED, 1); 
    }
    else
    {
        digitalWrite(SYS_LED, 0); 
    }

    LedCodeIndex++;
    if (LedCodeIndex >= LED_CODE_SIZE) /* Range check to find out when end of LED pattern has been reached*/
    {
        LedCodeIndex = 0;
        for (uint8_t bit = BitIndex; bit < 8; bit++)
        {
          if (SetLedCode & (1 << bit)) /* Check if next bit is set. If so, set next LED pattern to set bit*/
          {
              LedCode = BitIndex;   
              breakForLoop = true; /* break for loop so that next bit is not checked until this LED code is shown */
          }
          BitIndex++;
          if (BitIndex >= 8) /* Range check to find out when end of LedCode has been reached*/
          {
              BitIndex = 0;
              LedCode = 0;  /* Set to off for one code cycle */
              //SetLedCode = 7; /* Set code to last to signify exit ennunciation */
          }
          if (breakForLoop == true) 
          {
            break;
          }
        }
    }
}

/***********************************************************************************************//**
 * @details     Set LED code. Updates the LED display to reflect the bits set in the input code.
 * @param[in]   led_code  The LED code value to be displayed.
 **************************************************************************************************/
void LED_App :: Set_LED_Code(uint8_t led_code)
{
    SetLedCode = led_code;
    Serial.print("led_code VAL: 0x");
    Serial.println(led_code, HEX);
}

/***********************************************************************************************//**
 * @details     Toggle LED
 **************************************************************************************************/
void LED_App :: Toggle_LED(void)
{
    digitalWrite(SYS_LED, !digitalRead(SYS_LED));
}

