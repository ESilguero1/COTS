#ifndef CONTROL_H

/* ========================================================================
   $File: Control.h$
   $Date: $
   $Revision: $
   $Creator:  $
   $Email:  $
   $Notice: $
   ======================================================================== */

#define CONTROL_H
#include "System_definitions.h"
#include "Joystick.h"
#include "MotorControl.h"

struct flags
{
	public:
		bool isJSEnable 		= true;
		bool isSeeking			= false;
		bool direction			= true;
		bool isPositioning		= false;
};

class CombinedControl {
	public:

		// Class Function
      CombinedControl();
      void begin();

      //================= JOYSTICK CONTROL FUNCTIONS ===============

		void enableJoystick();
      void disableJoystick();

      //=================== MOTOR CONTROL FUNCTIONS =================

      //===== MOVE FUNCTIONS =====

      void goPos(uint8_t motor_id, signed long position);                                  // brings the motor back to its home position
      void setHome(uint8_t motor_id);                                                      // sets the home position using the right hand switch
      void forward(uint8_t motor_id, unsigned long stepsForward, unsigned long velocity);    // push forward at the specified velocity
      void reverse(uint8_t motor_id, unsigned long stepsBackward, unsigned long velocity);   // moves motor in reverse direction
      void constForward(uint8_t motor_id, unsigned long velocity);                           // moves the motor forward constantly
      void constReverse(uint8_t motor_id, unsigned long velocity);                           // moves the motor backwards constantly
      void stop(uint8_t motor_id);                                                         // stops the motor when it is in continuous movement
      bool seek(uint8_t motor_id, bool goForward);                                           // goes until a switch as defined by goForward is pressed

      //===== INFO FUNCTIONS =====

      void status(uint8_t motor_id, int * statusBits);                                       // returns the status bits of the motor
      unsigned long sgStatus(uint8_t motor_id);                                            // returns the stallguard status info

      bool standstill(uint8_t motor_id);                                                   // checks if the motor is at a standstill
      void Setstandstill(uint8_t motor_id, bool state);                                    // sets motor standstill value

      double getXactual(uint8_t motor_id);                                                 // returns the position of the motor
      unsigned long getVelocity(uint8_t motor_id);                                         // returns the vmax speed
      unsigned long getAcceleration(uint8_t motor_id);                                     // returns the amax acceleration
      unsigned long getDeceleration(uint8_t motor_id);                                     // returns the dmax deceleration
      unsigned long getPower(uint8_t motor_id);                                            // returns the running power

      //===== SET FUNCTIONS =====

      void setVelocity(uint8_t motor_id, unsigned long velocity);                            // sets the vmax velocity
      void setAcceleration(uint8_t motor_id, unsigned long acceleration);                    // sets the amax acceleration
      void setDeceleration(uint8_t motor_id, unsigned long deceleration);                    // sets the dmax deceleration
      void setPower(uint8_t motor_id, unsigned long holdPower, unsigned long runPower);      // sets the hold and run power
      void setXtarget(uint8_t motor_id, unsigned long position);                             // sets the target position (will move in mode 0)
      void setResolution(uint8_t motor_id, int resolution);                                  // sets the step resolution of the motor

      void changePosNoMove(uint8_t motor_id, unsigned long position);                        // changes the actual position value without moving
      void setDirections(uint8_t motor_id, bool forwardDirection, bool forwardSwitch);       // sets the dir of switches and which is the forward dir
      void switchActiveEnable(uint8_t motor_id, bool fw, bool bw);                           // allows the user to change switches active high or low
      void SetSlowFastJoyStick(uint8_t slow_fast);
      void SetMirrorMode(uint8_t mirror_mode ) ;
      void SetJSControlMode(uint8_t js_cntrl_mode) ;
      uint8_t GetJSControlMode(void) ;

   private:
      MotorControl motor[3];
      Joystick joystick;

      int _seekStep;
      int _stepResolution;
      int _resolutionNum;
      double _lastX_Y_vel[3];
      unsigned long _lastRead;
      bool _mirrorMode;
      bool _slow_fast;
      bool _mtr3JSControl;


      void _setJS(uint8_t motor_id, double velocity);
      bool _timer(unsigned long lastReadTime);
};

#endif
