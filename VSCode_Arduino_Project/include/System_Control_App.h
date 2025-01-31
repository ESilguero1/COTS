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
#include "System_definitions.h"
#include <SPI.h>
#include "Arduino.h"
#include "CmdMessenger.h"

class System_Control_App 
{

	public:
		// Class Function
      void Init();
	   void ServiceSystemResponseApp();
};
#endif
