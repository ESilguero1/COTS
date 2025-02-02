
#include "System_definitions.h"
#include "debugutils.h"
#include "CombinedControl.h"
#include "Adafruit_BLE_App.h"
#include "System_Control_App.h"
#include "BNO080_App.h"

Adafruit_BLE_App BLE_App;
System_Control_App SystemControlApp;
BNO080_App BNO080App;

uint8_t InitMotorState= 0;

// =============== Arduino Functions and Calls ===============

void setup()
{
	SystemControlApp.Init();

	if ( SystemControlApp.RequestMotorStatus(0) != 12582996)
	{
		InitMotorState |= (1<<INIT_MOTOR1_STAT_FAILED);
	}

	if (SystemControlApp.RequestMotorStatus(1) != 12582996)
	{
		InitMotorState |= (1<<INIT_MOTOR2_STAT_FAILED);
	}
 	if (SystemControlApp.RequestMotorStatus(2) != 12582996)
	{
		InitMotorState |= (1<<INIT_MOTOR3_STAT_FAILED);
	} 

	if (BLE_App.Init() != 0)
	{
		InitMotorState |= (1<<INIT_BLE_STAT_FAILED);
	}
	if (BNO080App.Init() != 0)
	{
		InitMotorState |= (1<<INIT_BNO80_STAT_FAILED);
	}

}

void loop()
{
	if (InitMotorState < (1<<INIT_MOTOR2_STAT_FAILED))
	{
		SystemControlApp.ServiceSystemResponseApp();
		BLE_App.Service_BLE_UART();
		BNO080App.Service_BNO080();	
	}
}
