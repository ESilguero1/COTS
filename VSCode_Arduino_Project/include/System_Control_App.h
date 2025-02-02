#ifndef System_Control_App_H
/* ========================================================================
   $File: System_Control_App.h$
   $Date: $
   $Revision: $
   $Creator:  $
   $Email:  $
   $Notice: $
   ======================================================================== */

#define System_Control_App_H
#include <SPI.h>
#include "Arduino.h"

class System_Control_App 
{

	public:
		// Class Function
      void Init();
	   void ServiceSystemResponseApp();
      uint32_t RequestMotorStatus(uint8_t target_motor);
};

#endif
