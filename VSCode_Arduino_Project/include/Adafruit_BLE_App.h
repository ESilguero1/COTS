#ifndef Adafruit_BLE_App_H
/* ========================================================================
   $File: Adafruit_BLE_App.h$
   $Date: $
   $Revision: $
   $Creator:  $
   $Email:  $
   $Notice: $
   ======================================================================== */

#define Adafruit_BLE_App_H
#include "System_definitions.h"
#include <SPI.h>
#include "Arduino.h"
class Adafruit_BLE_App 
{

	public:
		// Class Function
      uint8_t Init();
	   void Service_BLE_UART();
};
#endif
