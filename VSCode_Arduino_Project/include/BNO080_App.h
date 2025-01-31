#ifndef BNO080_App_H
/* ========================================================================
   $File: BNO080_App.h$
   $Date: $
   $Revision: $
   $Creator:  $
   $Email:  $
   $Notice: $
   ======================================================================== */

#define BNO080_App_H
#include "System_definitions.h"
#include <SPI.h>
#include "Arduino.h"
class BNO080_App 
{

	public:
		// Class Function
      uint8_t Init();
	   void Service_BNO080();
};
#endif
