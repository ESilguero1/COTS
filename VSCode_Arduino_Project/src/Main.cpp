
#include "System_definitions.h"
#include "debugutils.h"
#include "CombinedControl.h"
#include "Adafruit_BLE_App.h"
#include "System_Control_App.h"
#include "BNO080_App.h"


Adafruit_BLE_App BLE_App;
System_Control_App SystemControlApp;
BNO080_App BNO080App;

// =============== Arduino Functions and Calls ===============

void setup()
{
	SystemControlApp.Init();
	BLE_App.Init();
	BNO080App.Init();
}

void loop()
{
	SystemControlApp.ServiceSystemResponseApp();
	BLE_App.Service_BLE_UART();
	BNO080App.Service_BNO080();

}
