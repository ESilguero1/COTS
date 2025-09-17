/***********************************************************************************************//**
 * @file       Main.cpp
 * @details    Main COTS application. Schedule system services tasks
 * @author      Miguel Silguero
 * @copyright  Copyright (c) 2024-2025, Horizonless Embedded Solutions LLC
 * @date       08.06.2024 (created)
 *
 **************************************************************************************************/

/***************************************************************************************************
 * INCLUDES
 **************************************************************************************************/
#include "System_definitions.h"
#include "debugutils.h"
#include "CombinedControl.h"
#include "Adafruit_BLE_App.h"
#include "System_Control_App.h"
#include "BNO080_App.h"
#include "LED_App.h"
#include <AsyncTask.h>

/***************************************************************************************************
 * CONSTANTS AND DEFINITIONS
 **************************************************************************************************/

/***************************************************************************************************
 * MODULE VARIABLES
 **************************************************************************************************/
AsyncTask asyncTask;                 /* Handles asynchronous tasks */
BNO080_App BNO080App;                /* IMU application object */
Adafruit_BLE_App BLE_App;            /* Bluetooth application object */
System_Control_App SystemControlApp; /* System control application object */
LED_App LEDApp;                      /* IMU application object */
uint8_t SystemInitState = 0; /* Tracks initialization status of the system */

/***************************************************************************************************
 * PRIVATE FUNCTION PROTOTYPES
 **************************************************************************************************/
void Mtr3PowerDisableCheck(void);   /* Checks if motor 3 should be powered down */
void ServiceIMUapp(void);           /* Handles periodic IMU data servicing */
void ServiceLEDapp(void);           /* Handles periodic LED service */
void ServiceJSswitch(void);

/***********************************************************************************************//**
 * @details     Setup for main program. Initialize all subsystems and report any failures
 **************************************************************************************************/
void setup()
{
    /* disable all motors */
    pinMode(MTR_ENA_0, OUTPUT);
    pinMode(MTR_ENA_1, OUTPUT);
    pinMode(MTR_ENA_2, OUTPUT);
    digitalWrite(MTR_ENA_2, HIGH);
    digitalWrite(MTR_ENA_0, HIGH);
    digitalWrite(MTR_ENA_1, HIGH);
    delay(200);
    LEDApp.Init();

    if (BNO080App.Init() != 0)              /* Initialize BNO080 IMU module */
    {
        SystemInitState |= (1 << INIT_BNO80_STAT_FAILED);
    }

    SystemControlApp.Init();                /* Initialize system control */

    /* Check motor initialization status and set failure flags if needed */
    if (SystemControlApp.RequestMotorStatus(0) != MOTOR_OK_STATUS[0])
    {
        SystemInitState |= (1 << INIT_MOTOR1_STAT_FAILED);
    }
    if (SystemControlApp.RequestMotorStatus(1) != MOTOR_OK_STATUS[1])
    {
        SystemInitState |= (1 << INIT_MOTOR2_STAT_FAILED);
    }
    if (SystemControlApp.RequestMotorStatus(2) != MOTOR_OK_STATUS[2])
    {
        SystemInitState |= (1 << INIT_MOTOR3_STAT_FAILED);
    }

    if (BLE_App.Init() != 0) /* Initialize Bluetooth Low Energy (BLE) module */
    {
       SystemInitState |= (1 << INIT_BLE_STAT_FAILED);
    }

    /* enable all, but motor 3 on startup */
    digitalWrite(MTR_ENA_0, LOW);
    digitalWrite(MTR_ENA_1, LOW);
    digitalWrite(MTR_ENA_2, HIGH);

    SystemControlApp.SetSysInitstate(SystemInitState);               /* Prep Report intialization state*/
    LEDApp.Set_LED_Code(SystemInitState);                           /* Enunciate system intialization state*/

    /* Schedule periodic tasks for motor power check and IMU servicing */
    asyncTask.repeat(Mtr3PowerDisableCheck, 1000);                  /* Check motor power every second */
    asyncTask.repeat(ServiceIMUapp, IMU_DATA_ACQUSITION_PERIOD);    /* Service IMU periodically */
    asyncTask.repeat(ServiceLEDapp, LED_FREQ_RATE_HZ);              /* Service LED periodically */
    //asyncTask.repeat(ServiceJSswitch, JS_SWITCH_CHK);              /* Service JS swtich periodically */
}

/***********************************************************************************************//**
 * @details     Main loop
 **************************************************************************************************/
void loop()
{
    SystemControlApp.ServiceSystemResponseApp();        /* Process system responses */
    BLE_App.Service_BLE_UART();                         /* Handle BLE communication */
    asyncTask.loop();                                   /* Execute other async scheduled tasks */
}

/***********************************************************************************************//**
 * @details     After motor 3 is enabled, this function checks if velocity is below
 a value before shutting the motor down.
 **************************************************************************************************/
void Mtr3PowerDisableCheck(void) 
{
	SystemControlApp.ServiceMotor3PowerDisable();
}

/***********************************************************************************************//**
 * @details     Service BNO080 application every IMU_DATA_ACQUSITION_PERIOD ms
 **************************************************************************************************/
void ServiceIMUapp(void)
{
    if ((SystemInitState & (1 << INIT_BNO80_STAT_FAILED)) == 0 ) /*ONly run if init passed for device*/
    {
        BNO080App.Service_BNO080();	
    }
}

/***********************************************************************************************//**
 * @details     Service LED application every LED_FREQ_RATE_HZ ms
 **************************************************************************************************/
void ServiceLEDapp(void)
{
    LEDApp.Service_LED();
}

/***********************************************************************************************//**
 * @details     Service Joystick mode switch debouncing every JS_SWITCH_CHK ms
 *  The the jostick mode determines if the joystick controls motor 3 only or ignores motor 3
 * and services motors 1 & 2
 **************************************************************************************************/
void ServiceJSswitch(void)
{
    static int lastJSstate = HIGH;
    int JSstate = PIOB->PIO_PDSR & PIO_PDSR_P27;

    if ((lastJSstate == LOW) & (JSstate = LOW))
    {
        SystemControlApp.ToggleJSmtrlControlMode();
    }
    lastJSstate = JSstate;

}