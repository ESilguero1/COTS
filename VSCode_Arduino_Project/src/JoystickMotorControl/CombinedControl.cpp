#include "CombinedControl.h"

double fast_slow_multiplier[3] = {1.0, 5.0,1.0};

/* ======================================================================
	Initializes the control object to control both the motor and the
	joystick.
 ====================================================================== */

CombinedControl :: CombinedControl() {

}

void CombinedControl :: begin() {

	// byte csPin, byte enablePin, int ID. The empty constructor cannot take the pins and so
	//they must be set after construction by the MotorControl :: set function.
	motor[0].set(MTR_CS0, MTR_ENA_0, 0); 
 	motor[1].set(MTR_CS1, MTR_ENA_1, 1); 
	motor[2].set(MTR_CS2, MTR_ENA_2, 2); 
 
	// double yrange, double ythreshold, int ypin, double xrange, double xthreshold, int xpin
	joystick.set(10000.0, 0.005 * 10000.0, JS_YAXIS_INPUT, 10000, 0.005 * 10000.0, JS_XAXIS_INPUT);

	_seekStep = 0;
	_stepResolution = 256;
  	_lastX_Y_vel[0] = 0;
  	_lastX_Y_vel[1] = 0;
	_lastX_Y_vel[2] = 0;
	_resolutionNum = 1;
	_mirrorMode = 0;
	_slow_fast = 0; 

	// Initializing the motor objects and start it at home position
	joystick.begin();
	motor[0].begin();
  	motor[1].begin();
	motor[2].begin();

	motor[2].stop();
	
	setPower(2,MTR3_HOLD_POWER,MTR3_RUN_POWER);
	setVelocity(2,STAND_MTR3_VELOCITY);
	setAcceleration(2, MTR3_ACCELERATION);
	

	// Print out motor data to confirm proper results
	Serial.print(motor[0].getMotorID());
	Serial.print(F(" : Motor 1 Data: "));
	motor[0].getMotorData();
	
 	Serial.print(motor[1].getMotorID());
	Serial.print(F(" : Motor 2 Data: "));
	motor[1].getMotorData();

 	Serial.print(motor[2].getMotorID());
	Serial.print(F(" : Motor 3 Data: "));
	motor[2].getMotorData();
	Serial.flush();
}

//====================================================================================
//====================== JOYSTCIK FUNCTIONS ==========================================
//====================================================================================

/* ======================================================================
	Function stops the joystick from controlling the motor and stops the 
	motor from running.
 ====================================================================== */

void CombinedControl :: disableJoystick() {
	motor[0].stop();
	motor[1].stop();
	motor[2].stop();
}

/* ======================================================================
	Function allows joystick to take over speed and direction controls.
		Up 		-> Increase speed, forward direction
		Down 	-> Decrease speed, backward direction
	Left and right are not currently configured.
 ====================================================================== */

void CombinedControl :: enableJoystick() 
{
	
	static bool firstRun = false;

	if (CombinedControl :: _timer(_lastRead)) 
	{
		_lastRead = millis();
		double X_Y_AxisVel[3] = {0,0,0};
		double LastVal =0.0;
		double PresentVal =0.0;
		static boolean X_Y_RampModeSet[3];

		X_Y_AxisVel[0] = joystick.xAxisControl() * fast_slow_multiplier[_slow_fast];// MS: Temporarily slowed down joystick to eliminate backlash
		X_Y_AxisVel[1] = joystick.yAxisControl() * fast_slow_multiplier[_slow_fast];
		//X_Y_AxisVel[2] = joystick.yAxisControl() * fast_slow_multiplier[_slow_fast];

		X_Y_AxisVel[2] = (joystick.yAxisControl() + joystick.yAxisControl())/ 2.0;
		if (firstRun == false)
		{
			firstRun = true;
			_lastX_Y_vel[2] = X_Y_AxisVel[2];
		}
	
		if (_mirrorMode == 1)
		{
			X_Y_AxisVel[1] = X_Y_AxisVel[1] * (-1.0);
		}


		#ifdef MOTOR_DEBUG
			Serial.print("X_AxisVel: ");
			Serial.println(X_Y_AxisVel[0]);
			Serial.print("X_AxisVel: ");
			Serial.println(X_Y_AxisVel[1]);
		#endif
		for (uint8_t axis = 2; axis < 3; axis++)
		{
			// check the direction of the velocity and past velocity
			if ( (_lastX_Y_vel[axis] >= 0) ^ (X_Y_AxisVel[axis] < 0) ) 
			{
				// if it exceeds a certain range, update the driving
				PresentVal = abs(X_Y_AxisVel[axis]);
				LastVal = abs(_lastX_Y_vel[axis]);

				if( ( PresentVal > ( LastVal * 1.25)) || ( PresentVal < (LastVal * 0.75) ) ) 
				{
					_lastX_Y_vel[axis] = X_Y_AxisVel[axis];
					CombinedControl :: _setJS(axis, X_Y_AxisVel[axis]);
					X_Y_RampModeSet[axis] = false;
					motor[axis].status_standstill = false;
					motor[axis].powerEnable();
				}
			}
			// always update motor if velocity and last velocity are in different directions
			else 
			{
				_lastX_Y_vel[axis] = X_Y_AxisVel[axis];
				CombinedControl :: _setJS(axis, X_Y_AxisVel[axis]);
				X_Y_RampModeSet[axis] = false;
				motor[axis].status_standstill = false;
				motor[axis].powerEnable();
			}

			if ((X_Y_RampModeSet[axis] == false) && (X_Y_AxisVel[axis] < 150.0))
			{
				X_Y_RampModeSet[axis] = true;
				//motor[axis].powerDisable();
				//motor[axis].setVelocity(STAND_MTR_VELOCITY);
			}
}
	}
}

/* ======================================================================
 	Sets the direction and speed of the motor as read from the joystick.
====================================================================== */

void CombinedControl :: _setJS(uint8_t motor_id, double velocity) {

	if ( velocity < 0 ) {
		motor[motor_id].constReverse(abs(velocity));
	}

	else {
		motor[motor_id].constForward(velocity);
	}
}

/* ======================================================================
 	Simple non-blocking timer to limit the amount of updates for reading
 	values from the joystick.
====================================================================== */

bool CombinedControl :: _timer(unsigned long lastReadTime) {
	bool done = false;
	unsigned long now = millis();
	if(now - lastReadTime > 400 ) {
		done = true;
	}
	return done;
}

//====================================================================================
//====================== MOVEMENT FUNCTIONS ==========================================
//====================================================================================

/* ======================================================================
	Function sends commands to the motor to move it to any state. Going to 
	position '0' will send it back to the home state.
 ====================================================================== */

void CombinedControl :: goPos(uint8_t motor_id, signed long position) 
{
	{
		if (motor_id == 0)
		{	
			if ((position < -4582400) || (position >= 4608000) ) //Limit range between -179 and 180 degrees
			{
			Serial.print("Limit range between -179 and 180 degrees exceeded");
			}
			else 
			{
				motor[motor_id].goPos(position);
			}
		}
		else if (motor_id == 1)
		{	
			if ((position < 0) || (position > 2304000) )
			{
			Serial.print("Limit range exceeded");
			}
			else 
			{
				motor[motor_id].goPos(position);
			}
		}
		else 
		{	
			motor[motor_id].goPos(position);
		}
	}
}

/* ======================================================================
	Function sends commands to the motor to return it to the "home" state.
	Function checks homing device in order and will not complete until all
	steps have been completed successfully.
 ====================================================================== */

void CombinedControl :: setHome(uint8_t motor_id) {
	bool setHome = false;
	while (!setHome) {
		setHome = motor[motor_id].setHome();
	}
}

/* ======================================================================
	Function sends commands to rotate the motor the number of half-steps
	specified in the forward direction. Returns true if the commands are 
	successfully sent.
 ====================================================================== */

void CombinedControl :: forward(uint8_t motor_id, unsigned long stepsForward, unsigned long velocity) {
	motor[motor_id].forward(stepsForward, velocity);
}

/* ======================================================================
	Function sends commands to rotate the motor the number of half-steps
	specified in the reverse direction. Returns true if the commands are 
	successfully sent.
 ====================================================================== */

void CombinedControl :: reverse(uint8_t motor_id, unsigned long stepsBackward, unsigned long velocity) {
	motor[motor_id].reverse(stepsBackward, velocity);
}

/* ======================================================================
	Function sends commands to rotate the motor continuously forwards at a
	constant velocity
 ====================================================================== */

void CombinedControl :: constForward(uint8_t motor_id, unsigned long velocity) {
	motor[motor_id].constForward(velocity);
}

/* ======================================================================
	Function sends commands to rotate the motor continuously backwards at a
	constant velocity
 ====================================================================== */

void CombinedControl :: constReverse(uint8_t motor_id, unsigned long velocity) {
	motor[motor_id].constReverse(velocity);
}

/* ======================================================================
	Function sends commands to stop the motor when it is in a constant reverse
	or in a constant forward movement.
 ====================================================================== */

void CombinedControl :: stop(uint8_t motor_id) {
	motor[motor_id].stop();
	motor[motor_id].powerDisable();
}

/* ======================================================================
	Seeks a stop event such that a button on the left or right is pressed
	to indicate the stopping of the motor[motorID].
 ====================================================================== */

bool CombinedControl :: seek(uint8_t motor_id, bool goForward) {

	bool done = false;

	switch(_seekStep) {

		// Set the direction of seeking
		case(0): {
			if (goForward == true) {
				motor[motor_id].constForward(STAND_MTR_VELOCITY / _resolutionNum);
				_seekStep = 1;
			}
			else {
				motor[motor_id].constReverse(STAND_MTR_VELOCITY / _resolutionNum);
				_seekStep = 2;
			}
		} break;

		// check if right button (goForward = true) is pressed
		case(1): {
			motor[motor_id].buttonStatus();
			if (motor[motor_id].forwardSwitch == true) {
				_seekStep = 3;
			}
		} break;

		// check if left button (goForward = false) is pressed
		case(2): {
			motor[motor_id].buttonStatus();
			if (motor[motor_id].backwardSwitch == true) {
				_seekStep = 3;
			}
		} break;

		// stop the motor and finish seeking
		default: {
			motor[motor_id].stop();
			_seekStep = 0;
			done = true;
		}
	}
	return done;
}

//====================================================================================
//==================== INFORMATION FUNCTIONS =========================================
//====================================================================================

/* ======================================================================
	Gets the current status of the motor and sends it back as an integer
	array of 1's and 0's.
 ====================================================================== */

void CombinedControl :: status(uint8_t motor_id, int * statusBits) {

	motor[motor_id].readStatus();

	statusBits[0] = motor[motor_id].status_sg2;
	statusBits[1] = motor[motor_id].status_sg2_event;
	statusBits[2] = motor[motor_id].status_standstill;

	statusBits[3] = motor[motor_id].status_velocity_reached;
	statusBits[4] = motor[motor_id].status_position_reached;
	statusBits[5] = motor[motor_id].status_position_reached_event;

	statusBits[6] = motor[motor_id].status_stop_l;
	statusBits[7] = motor[motor_id].status_stop_r;
	statusBits[8] = motor[motor_id].status_stop_l_event;
	statusBits[9] = motor[motor_id].status_stop_r_event;
	statusBits[10] = motor[motor_id].status_latch_l;
	statusBits[11] = motor[motor_id].status_latch_r;

	statusBits[12] = motor[motor_id].status_openLoad_A;
	statusBits[13] = motor[motor_id].status_openLoad_B;
	statusBits[14] = motor[motor_id].status_shortToGround_A;
	statusBits[15] = motor[motor_id].status_shortToGround_B;

	statusBits[16] = motor[motor_id].status_overtemperatureWarning;
	statusBits[17] = motor[motor_id].status_overtemperatureShutdown;

	statusBits[18] = motor[motor_id].status_isReverse;
	statusBits[19] = motor[motor_id].status_resetDetected;
	statusBits[20] = motor[motor_id].status_driverError;
	statusBits[21] = motor[motor_id].status_underVoltage;

	statusBits[22] = motor[motor_id].getIsForward();
	statusBits[23] = motor[motor_id].getIsPositionMode();
	statusBits[24] = motor[motor_id].getIsHomed();
}

/* ======================================================================
	Gets the current status of the drive status register, looking at only the
	stall guard bits and sends it back as an integer array of 1's and 0's.
 ====================================================================== */

unsigned long  CombinedControl :: sgStatus(uint8_t motor_id) {
	motor[motor_id].sgStatus();
	return motor[motor_id].sgStatusBits;
}

/* ======================================================================
	Checks and confirms if the motor comes to a standstill
 ====================================================================== */

bool CombinedControl :: standstill(uint8_t motor_id)
 {
	return motor[motor_id].status_standstill;
}
/* ======================================================================
	Sets motor standstill value
 ====================================================================== */

 void CombinedControl :: Setstandstill(uint8_t motor_id, bool state) 
{
	motor[motor_id].status_standstill = state;
}

/* ======================================================================
	Returns the actual position of the motor[motor_id].
 ====================================================================== */

double CombinedControl :: getXactual(uint8_t motor_id) {
	double xPos = motor[motor_id].getXactual();
	if (xPos > 2147483648.0) {
		Serial.println(xPos);
		xPos -= 4294967296.0;
		Serial.println(xPos);
	}
	return (xPos);
}

/* ======================================================================
	Gets the maximum velocity to the given value
 ====================================================================== */

unsigned long CombinedControl :: getVelocity(uint8_t motor_id) {
	return motor[motor_id].getVelocity();
}

/* ======================================================================
	Gets the acceleration from maximum velocity to the stop velocity
	to the given value.
 ====================================================================== */

unsigned long CombinedControl :: getAcceleration(uint8_t motor_id) {
	return motor[motor_id].getAcceleration();
}

/* ======================================================================
	Gets the deceleration from maximum velocity to the stop velocity
	to the given value.
 ====================================================================== */

unsigned long CombinedControl :: getDeceleration(uint8_t motor_id) {
	return motor[motor_id].getDeceleration();
}

/* ======================================================================
	Gets the deceleration from maximum velocity to the stop velocity
	to the given value.
 ====================================================================== */

unsigned long CombinedControl :: getPower(uint8_t motor_id) {
	return motor[motor_id].getPowerLevel();
}

//====================================================================================
//====================== SETTER FUNCTIONS ============================================
//====================================================================================

/* ======================================================================
	Changes the maximum velocity to the given value
 ====================================================================== */

void CombinedControl :: setVelocity(uint8_t motor_id, unsigned long velocity) {
	motor[motor_id].setVelocity(velocity);
}

/* ======================================================================
	Changes the acceleration from maximum velocity to the stop velocity
	to the given value.
 ====================================================================== */

void CombinedControl :: setAcceleration(uint8_t motor_id, unsigned long acceleration) {
	motor[motor_id].setAcceleration(acceleration);
}

/* ======================================================================
	Changes the deceleration from maximum velocity to the stop velocity
	to the given value.
 ====================================================================== */

void CombinedControl :: setDeceleration(uint8_t motor_id, unsigned long deceleration) {
	motor[motor_id].setDeceleration(deceleration);
}

/* ======================================================================
	Changes the deceleration from maximum velocity to the stop velocity
	to the given value.
 ====================================================================== */

void CombinedControl :: setPower(uint8_t motor_id, unsigned long holdPower, unsigned long runPower) {
	motor[motor_id].setPowerLevel(holdPower, runPower);
}

/* ======================================================================
	Changes xtarget (where the motor will go, this starts a motion if xtarget
	is not the same as xactual in the positioning mode ie: ramp mode = 0) value 
	to the given position.
 ====================================================================== */

void CombinedControl :: setXtarget(uint8_t motor_id, unsigned long position) {
	motor[motor_id].setXtarget(position);
}

/* ======================================================================
	Sets the CHOPCONF register to set the resolution of the motor[motor_id]. The
	default value is 256.
 ====================================================================== */

void CombinedControl :: setResolution(uint8_t motor_id, int resolution) {
	motor[motor_id].setChopConf(resolution);
	_resolutionNum = motor[motor_id].getResolution();
}

/* ======================================================================
	Changes the position of the motor without moving the motor by resetting
	the current position to the specified position.
 ====================================================================== */

void CombinedControl :: changePosNoMove(uint8_t motor_id, unsigned long position) {
	motor[motor_id].stop();

	motor[motor_id].setRampMode(1);

	motor[motor_id].setXactual(position);
	motor[motor_id].setXtarget(position);
}

/* ======================================================================
	Sets the direction of the switches and the motor relative to each other.
	When the values are set (shown below) the given value is the switch set forward
	and the direction set forward
	DIREC. MODE 	F_SWITCH	B_SWITCH	FOR_MOTOR	BACK_MOTOR
		1			right		left 		cw 			ccw
		2			right 		left 		ccw 		cw
		3			left 		right 		cw 			ccw
		4			left 		right 		ccw 		cw
====================================================================== */

void CombinedControl :: setDirections(uint8_t motor_id, bool forwardDirection, bool forwardSwitch) {
	motor[motor_id].swapDirection(!forwardDirection, !forwardSwitch);
}

/* ======================================================================
	Sets the active state of the left and right switches. Setting to 1 
	(true) is active low and setting to 0 is active high.
====================================================================== */

void CombinedControl :: switchActiveEnable(uint8_t motor_id, bool fw, bool bw) {
	motor[motor_id].switchActiveEnable(fw, bw);
}

/* ======================================================================
	Changes the velocity of joystick for faster movement
 ====================================================================== */

void CombinedControl :: SetSlowFastJoyStick(uint8_t slow_fast) 
{
      _slow_fast = slow_fast;
}

/* ======================================================================
	enables mirror mode
 ====================================================================== */

void CombinedControl :: SetMirrorMode(uint8_t mirror_mode ) 
{
      _mirrorMode = mirror_mode;
}