/***********************************************************************************************//**
 * @file       BNO080_App.cpp
 * @details    System control application module. Motors and joystick initialization.
 *             Services serial communication requests and responses.
 * @author		Miguel Silguero
 * @copyright  Copyright (c) 2024-2025, Horizonless Embedded Solutions LLC
 * @date       12.01.2024 (created)
 *
 **************************************************************************************************/

/***************************************************************************************************
 * INCLUDES
 **************************************************************************************************/
#include "System_Control_App.h"
#include "System_definitions.h"
#include "CmdMessenger.h"
#include "debugutils.h"
#include "CombinedControl.h"
#include <SPI.h>
#include "Adafruit_BLE_App.h"

/***************************************************************************************************
 * CONSTANTS AND DEFINITIONS
 **************************************************************************************************/

/***************************************************************************************************
 * MODULE VARIABLES
 **************************************************************************************************/
CmdMessenger cmdMessenger = CmdMessenger(Serial); // Serial communication handler
String outputStr; // String buffer for serial output
CombinedControl control; // Object for managing motor and joystick control
flags motorFlags[3]; // Flags for motor status tracking
int status[25]; // Array for storing system status data
extern union floatUnion AveragedIMUdata[6];
extern uint32_t IMU_Comm_Errors;
Adafruit_BLE_App BLE_App_sys;            /* Bluetooth application object */
uint8_t SysInitState = 0; /* Report initialization status of the system */

/***************************************************************************************************
 * PRIVATE FUNCTION PROTOTYPES
 **************************************************************************************************/
void attachCommandCallbacks(); /* Attach command handlers */
void _checkJS(uint8_t motorID); /* Check joystick input for a specific motor */
void OnUnknownCommand(); /* Handler for unknown serial commands */
void onRequestMotorStatus(); /* Request motor status */
void onRequestStallStatus(); /* Request stall status */
void onRequestSetPosNoMove(); /* Set motor position without movement */
void onSetSlowFastJSMotion(); /* Adjust joystick motion sensitivity */
void onSetJSMirrorMode(); /* Set joystick mirror mode */
void onGetXactual(); /* Get motor position */
void onGetVelocity(); /* Get motor velocity */
void onGetAcceleration(); /* Get motor acceleration */
void onGetDeceleration(); /* Get motor deceleration */
void onGetPower(); /* Get motor power consumption */
void onGetIMUData(); /* Collected averaged IMU data */

void onConstantForward(); /* Move motor at a constant speed forward */
void onConstantBackward(); /* Move motor at a constant speed backward */
void onMovePosition(); /* Move motor to a specified position */
void onMoveForward(); /* Move motor forward */
void onMoveBackward(); /* Move motor backward */
void onVelocity(); /* Set motor velocity */
void onAcceleration(); /* Set motor acceleration */
void onDeceleration(); /* Set motor deceleration */
void onPower(); /* Set motor power */
void onDirection(); /* Set motor direction */
void onJStoggleCntl(); /* Toggle joystick motorl control */
void onJSEnable(); /* Enable/Disable joystick control */
void onJSDisable(); /* Disable joystick control */
void onMotorStop(); /* Stop the motor */
void onMotorHome(); /* Move motor to home position */
void onSeek(); /* Seek operation for motor */
void onResolution(); /* Set resolution settings */
void onActiveSettings(); /* Get active settings */
void onPing(); /* Ping command handler */

/***************************************************************************************************
 * FUNCTION DEFINITIONS
 **************************************************************************************************/

/***********************************************************************************************//**
 * @details     Initialize joystick and motor control pins, SPI, and motor controller.
 **************************************************************************************************/
void System_Control_App :: Init(void)
{
    /* =============== Setup Motor Control =============== */

    /* Configure enable and Chip Select pins for all motors */
    pinMode(MTR_CS0, OUTPUT);
    pinMode(MTR_CS1, OUTPUT);
    pinMode(MTR_CS2, OUTPUT);
    pinMode(MTR_ENA_0, OUTPUT);
    pinMode(MTR_ENA_1, OUTPUT);
    pinMode(MTR_ENA_2, OUTPUT);
    
    /* enable all, but motor 3 on startup */
    digitalWrite(MTR_ENA_0, 0);
    digitalWrite(MTR_ENA_1, 0);
    digitalWrite(MTR_ENA_2, 1);
    
    /* Configure joystick stop switch */
	PIOB->PIO_PER=(1<<27); //Enable PIO for GPIO P13
	PIOB->PIO_OER=(1<<27); //Enable port pin for output P13
    pinMode(JS_STOP_SWTICH, INPUT_PULLUP);

	Serial.begin(115200); /* Initialize serial communication */
    Serial.println("starting the coolest project in the history of mankind");
	
    /* Initialize SPI interface with appropriate settings */
    SPI.setBitOrder(MSBFIRST);
    SPI.setClockDivider(SPI_CLOCK_DIV8);
    SPI.setDataMode(SPI_MODE3);
    SPI.begin();

    /* =============== Initialize Objects =============== */

    /* Initialize motor and sensor control objects*/ 
    control.begin();

    /* Reserve memory for output string */ 
    outputStr.reserve(128);
    
    /* Ensure proper line endings for serial communication */ 
    cmdMessenger.printLfCr();
    
    /* Attach command callbacks for serial communication */ 
    attachCommandCallbacks();    
}

/***********************************************************************************************//**
 * @details     Service incoming serial messages and handle motor control logic.
 **************************************************************************************************/
void System_Control_App :: ServiceSystemResponseApp(void)
{
    /* Process incoming serial messages */ 
    cmdMessenger.feedinSerialData();

    /*  Enable joystick control if the flag is set */ 
    if (motorFlags[0].isJSEnable)
    {
        control.enableJoystick();
    }

    // Loop through all motors and process their status
    for (uint8_t motor = 0; motor < 3; motor++)
    {
        /* Handle seek operation for motors*/
        if (motorFlags[motor].isSeeking)
        {
            motorFlags[motor].isSeeking = !control.seek(motor, motorFlags[motor].direction);
        }

        /* Handle positioning completion for  */
        if (motorFlags[motor].isPositioning)
        {
            motorFlags[motor].isPositioning = !control.standstill(motor);
        }
    }
}

/***********************************************************************************************//**
 * @details     Checks velocity to confirm motor 3 has stopped before disabling it.
 **************************************************************************************************/
void System_Control_App :: ServiceMotor3PowerDisable(void)
{
    unsigned long velocity = 0;
    
    /* Get current velocity of motor 3*/
    velocity = control.getVelocity(2);

    /* If motor 3 is moving at very low speed, but hasn't stopped, stop it.*/
    if ((velocity < 160) && (velocity != 0))
    {
        control.stop(2); /*stop motor 3*/
    }
}

/***********************************************************************************************//**
 * @details     Collects motor status information bits
 **************************************************************************************************/
uint32_t  System_Control_App :: RequestMotorStatus(uint8_t target_motor)
{
	uint32_t motorStat=0;
	outputStr.remove(0);

	control.status(target_motor, &status[0]);

	for (int i = 0; i < MTR_STATUS_SIZE; i++)
	{
		motorStat |= status[i]<<i; /* Rebuild hex value */
	}

	outputStr.concat(motorStat); /* print out for debug only */
	outputStr.concat(F(";"));
	Serial.println(outputStr);
	BLE_App_sys.println(outputStr);

	return motorStat;
}



/***********************************************************************************************//**
 * @details     
 **************************************************************************************************/
void  System_Control_App :: SetSysInitstate(uint8_t state)
{
	SysInitState = state; /* Report initialization status of the system */
}


/***********************************************************************************************//**
 * @details     Toggle joystick control mode
 **************************************************************************************************/
void ToggleJSmtrlControlMode(void)
{
	control.SetJSControlMode(!control.GetJSControlMode());
}

// =============== Command Callbacks ===============

void attachCommandCallbacks()
{
	cmdMessenger.attach(OnUnknownCommand); // Reply: e,
	cmdMessenger.attach(REQUEST_MOTOR_STATUS, onRequestMotorStatus); // Reply: m,
	cmdMessenger.attach(REQUEST_SG_STATUS, onRequestStallStatus);	 // Reply: g,
	cmdMessenger.attach(REQUEST_POS_NO_MOVE, onRequestSetPosNoMove); // Reply: d,
	cmdMessenger.attach(SET_JS_SLOW_FAST, onSetSlowFastJSMotion); // Reply: A,
	cmdMessenger.attach(SET_JS_MIRROR_MODE, onSetJSMirrorMode);	  // Reply: V,
	cmdMessenger.attach(GET_XACTUAL, onGetXactual);				  // Reply: x,
	cmdMessenger.attach(GET_VELOCITY, onGetVelocity);			  // Reply: v,
	cmdMessenger.attach(GET_ACCELERATION, onGetAcceleration);	  // Reply: a,
	cmdMessenger.attach(GET_DECELERATION, onGetDeceleration);	  // Reply: d,
	cmdMessenger.attach(GET_POWER, onGetPower);					  // Reply: P,
	cmdMessenger.attach(GET_IMU_AVE_DATA, onGetIMUData); 		// Reply: i;

	cmdMessenger.attach(SET_CONST_FW, onConstantForward);  // Reply: S,1;
	cmdMessenger.attach(SET_CONST_BW, onConstantBackward); // Reply: S,1;
	cmdMessenger.attach(SET_MOVE_POS, onMovePosition);	   // Reply: S,1;
	cmdMessenger.attach(SET_MOVE_FW, onMoveForward);	   // Reply: S,1;
	cmdMessenger.attach(SET_MOVE_BW, onMoveBackward);	   // Reply: S,1;
	cmdMessenger.attach(SET_VELOCITY, onVelocity);		   // Reply: S,1;
	cmdMessenger.attach(SET_ACCELERATION, onAcceleration); // Reply: S,1;
	cmdMessenger.attach(SET_DECELERATION, onDeceleration); // Reply: S,1;
	cmdMessenger.attach(SET_POWER, onPower);			   // Reply: S,1;
	cmdMessenger.attach(SET_DIRECTION, onDirection);	   // Reply: S,0;

	cmdMessenger.attach(JS_TOGGLE_CNTRL, onJStoggleCntl);			   // Reply: S,1;
	cmdMessenger.attach(JS_ENABLE, onJSEnable);		   // Reply: S,1;
	cmdMessenger.attach(MOTOR_STOP, onMotorStop);		   // Reply: S,1;
	cmdMessenger.attach(MOTOR_HOME, onMotorHome);		   // Reply: S,1;
	cmdMessenger.attach(SEEK, onSeek);					   // Reply: S,1;
	cmdMessenger.attach(RESOLUTION, onResolution);		   // Reply: S,1;
	cmdMessenger.attach(ACTIVESETTINGS, onActiveSettings); // Reply: S,1;
	cmdMessenger.attach(PCPING, onPing);				   // Reply: p,PONG;
	
}

// ADD IF SWITCHES ARE HIT DURING GO TO SPECIFIC POSITION, THEN STOP RATHER THAN CONTINUEING ON

// =============== Additional Helper Functions ===============

bool hasExpired(unsigned long &prevTime, unsigned long interval)
{
	if (millis() - prevTime > interval)
	{
		prevTime = millis();
		return true;
	}
	return false;
}

void onSuccess()
{
	outputStr.remove(0);
	outputStr.concat(F("S,1;"));
	Serial.println(outputStr);
	BLE_App_sys.println(outputStr);
}

void onFail()
{
	outputStr.remove(0);
	outputStr.concat(F("S,0;"));
	Serial.println(outputStr);
	BLE_App_sys.println(outputStr);
}

bool _checkFlags(uint8_t motorID)
{
	bool done = false;
	_checkJS(motorID);
	if (!motorFlags[motorID].isSeeking && !motorFlags[motorID].isPositioning)
	{
		done = true;
	}
	return done;
}

void _checkJS(uint8_t motorID)
{
	if (motorFlags[motorID].isJSEnable)
	{
		onJSDisable();
	}
}

void _binaryDisplay(unsigned long status)
{
	unsigned long statusBit = 0;

	for (int i = 9; i > -1; i--)
	{
		statusBit = status >> i;
		statusBit = statusBit & 0x00000001;
		if (statusBit == 0)
		{
			outputStr.concat(F("0"));
		}
		else
		{
			outputStr.concat(F("1"));
		}
	}
}

// =============== Callback Functions ===============

// Format : outputStr = "e, unknown command;"
void OnUnknownCommand()
{
	outputStr.remove(0);
	outputStr.concat(F("e, unknown command;"));
	Serial.println(outputStr);
	BLE_App_sys.println(outputStr);
}

// Format : outputStr = "m,time,bit0,bit1,...,bit24;"
void onRequestMotorStatus()
{
	uint8_t target_motor = cmdMessenger.readInt16Arg();
	outputStr.remove(0);
	outputStr.concat(F("m,"));
	outputStr.concat(millis());

	control.status(target_motor, &status[0]);

	for (int i = 0; i < MTR_STATUS_SIZE; i++)
	{
		outputStr.concat(F(","));
		outputStr.concat(status[i]);
	}
	
	outputStr.concat(F(","));
	outputStr.concat(SysInitState);

	outputStr.concat(F(";"));
	Serial.println(outputStr);
	BLE_App_sys.println(outputStr);
}

// Format : outputStr = "g,time,status;"
void onRequestStallStatus()
{
	uint8_t target_motor = cmdMessenger.readInt16Arg();
	outputStr.remove(0);
	outputStr.concat(F("g,"));
	outputStr.concat(millis());
	outputStr.concat(F(","));

	_binaryDisplay(control.sgStatus(target_motor));

	outputStr.concat(F(";"));
	Serial.println(outputStr);
	BLE_App_sys.println(outputStr);
}

// Format : outputStr = "m,time,oldpos,newpos;"
void onRequestSetPosNoMove()
{
	uint8_t target_motor = cmdMessenger.readInt16Arg();
	outputStr.remove(0);
	outputStr.concat(F("d,"));
	outputStr.concat(millis());
	outputStr.concat(F(","));

	outputStr.concat(control.getXactual(target_motor));
	outputStr.concat(F(","));
	control.changePosNoMove(target_motor, cmdMessenger.readDoubleArg());
	outputStr.concat(control.getXactual(target_motor));

	outputStr.concat(F(";"));
	Serial.println(outputStr);
	BLE_App_sys.println(outputStr);
}

// Format : outputStr = "A,ADCBits;"
void onSetSlowFastJSMotion()
{
	uint8_t fast_slow_config = cmdMessenger.readInt16Arg();
	control.SetSlowFastJoyStick(fast_slow_config);
	outputStr.concat(F("fast_slow_config,"));
	outputStr.concat(fast_slow_config);
	outputStr.concat(F(";"));
	Serial.println(outputStr);
	BLE_App_sys.println(outputStr);
}

// Format : outputStr = "V,ADCVolts;"
void onSetJSMirrorMode()
{
	uint8_t mirror_mode_nfig = cmdMessenger.readInt16Arg();
	control.SetMirrorMode(mirror_mode_nfig);
	outputStr.concat(F("mirror_mode_nfig,"));
	outputStr.concat(mirror_mode_nfig);
	outputStr.concat(F(";"));
		Serial.println(outputStr);
	BLE_App_sys.println(outputStr);
}

// Format : outputStr = "x,time,xposition;" (Note that it is a 200 stepper motor)
void onGetXactual()
{
	uint8_t target_motor = cmdMessenger.readInt16Arg();
	outputStr.remove(0);
	outputStr.concat(F("x,"));
	outputStr.concat(millis());
	outputStr.concat(F(","));

	outputStr.concat(control.getXactual(target_motor));

	outputStr.concat(F(";"));
	Serial.println(outputStr);
	BLE_App_sys.println(outputStr);
}

// Format : outputStr = "v,time,velocity;"
void onGetVelocity()
{
	uint8_t target_motor = cmdMessenger.readInt16Arg();
	outputStr.remove(0);
	outputStr.concat(F("v,"));
	outputStr.concat(millis());
	outputStr.concat(F(","));

	outputStr.concat(control.getVelocity(target_motor));

	outputStr.concat(F(";"));
	Serial.println(outputStr);
	BLE_App_sys.println(outputStr);
}

// Format : outputStr = "a,time,acceleration;"
void onGetAcceleration()
{
	uint8_t target_motor = cmdMessenger.readInt16Arg();
	outputStr.remove(0);
	outputStr.concat(F("a,"));
	outputStr.concat(millis());
	outputStr.concat(F(","));

	outputStr.concat(control.getAcceleration(target_motor));

	outputStr.concat(F(";"));
	Serial.println(outputStr);
	BLE_App_sys.println(outputStr);
}

// Format : outputStr = "d,time,deceleration;"
void onGetDeceleration()
{
	uint8_t target_motor = cmdMessenger.readInt16Arg();
	outputStr.remove(0);
	outputStr.concat(F("d,"));
	outputStr.concat(millis());
	outputStr.concat(F(","));

	outputStr.concat(control.getDeceleration(target_motor));

	outputStr.concat(F(";"));
	Serial.println(outputStr);
	BLE_App_sys.println(outputStr);
}

// Format : outputStr = "P,time,power;"
void onGetPower()
{
	uint8_t target_motor = cmdMessenger.readInt16Arg();
	outputStr.remove(0);
	outputStr.concat(F("P,"));
	outputStr.concat(millis());
	outputStr.concat(F(","));

	outputStr.concat(control.getPower(target_motor));

	outputStr.concat(F(";"));
	Serial.println(outputStr);
	BLE_App_sys.println(outputStr);
}

// Format : outputStr = "i,IMU data;"
void onGetIMUData()
{
	char hex_string[11]; // Enough space for "0x" + 8 hex digits + null terminator

	outputStr.remove(0);
	outputStr.concat(F("imu,"));
	//outputStr.concat(millis());
	//outputStr.concat(F(","));

	for (uint8_t e = 0; e < 6; e++ )
	{
		sprintf(hex_string, "%x", AveragedIMUdata[e].i); /*Convert the hex value to a string*/
		outputStr.concat(hex_string);
		outputStr.concat(F(","));
	}

	outputStr.concat(IMU_Comm_Errors);
	outputStr.concat(F(";"));
	Serial.println(outputStr);
	BLE_App_sys.println(outputStr);
}

// Format : not changes to outputStr
void onConstantForward()
{
	uint8_t target_motor = cmdMessenger.readInt16Arg();
	if (_checkFlags(target_motor))
	{
		double velocity = cmdMessenger.readDoubleArg();
		if (velocity == 0)
		{
			onFail();
		}
		else
		{
			control.constForward(target_motor, velocity);
#ifdef DEBUG_COM
			Serial.print("Velocity: ");
			Serial.println(velocity);
#endif
			onSuccess();
		}
	}
	else
	{
		onFail();
	}
}

// Format : not changes to outputStr
void onConstantBackward()
{
	uint8_t target_motor = cmdMessenger.readInt16Arg();
	if (_checkFlags(target_motor))
	{
		double velocity = cmdMessenger.readDoubleArg();
		if (velocity == 0)
		{
			onFail();
		}
		else
		{
			control.constReverse(target_motor, velocity);
#ifdef DEBUG_COM
			Serial.print("Velocity: ");
			Serial.println(velocity);
#endif
			onSuccess();
		}
	}
	else
	{
		onFail();
	}
}

// Format : not changes to outputStr
void onMovePosition()
{
	uint8_t target_motor = cmdMessenger.readInt16Arg();
	_checkJS(target_motor);
	if (!motorFlags[target_motor].isSeeking)
	{
		double pos = cmdMessenger.readDoubleArg();
		control.goPos(target_motor, pos); // Note that the default if no argument read is to go home
#ifdef DEBUG_COM
		Serial.print("Position: ");
		Serial.println(pos);
#endif
		onSuccess();
	}
	else
	{
		onFail();
	}
}

// Format : not changes to outputStr
void onMoveForward()
{
	uint8_t target_motor = cmdMessenger.readInt16Arg();
	_checkJS(target_motor);
	if (!motorFlags[target_motor].isSeeking)
	{
		double stepsForward = cmdMessenger.readDoubleArg();
		double velocity = cmdMessenger.readDoubleArg();
		if (stepsForward == 0 || velocity == 0)
		{
			onFail();
		}
		else
		{
			control.forward(0, stepsForward, velocity);

#ifdef DEBUG_CO
			Serial.print("Velocity: ");
			Serial.println(velocity);
			Serial.print("Steps Forward: ");
			Serial.println(stepsForward);
#endif
			onSuccess();
		}
	}
	else
	{
		onFail();
	}
}

// Format : not changes to outputStr
void onMoveBackward()
{
	uint8_t targetMotor = cmdMessenger.readCharArg();
	_checkJS(targetMotor);
	if (!motorFlags[targetMotor].isSeeking)
	{
		double stepsForward = cmdMessenger.readDoubleArg();
		double velocity = cmdMessenger.readDoubleArg();
		if (stepsForward == 0 || velocity == 0)
		{
			onFail();
		}
		else
		{
			control.reverse(targetMotor, stepsForward, velocity);

#ifdef DEBUG_COM
			Serial.print("Velocity: ");
			Serial.println(velocity);
			Serial.print("Steps Forward: ");
			Serial.println(stepsForward);
#endif
			onSuccess();
		}
	}
	else
	{
		onFail();
	}
}

// Format : not changes to outputStr
void onVelocity()
{
	uint8_t target_motor = cmdMessenger.readInt16Arg();
	double velocity = cmdMessenger.readDoubleArg();

	if (velocity == 0)
	{
		onFail();
	}
	else
	{
		control.setVelocity(target_motor, velocity);
		onSuccess();
	}
}

// Format : not changes to outputStr
void onAcceleration()
{
	uint8_t target_motor = cmdMessenger.readInt16Arg();
	double acceleration = cmdMessenger.readDoubleArg();

	if (acceleration == 0)
	{
		onFail();
	}
	else
	{
		control.setAcceleration(target_motor, acceleration);
		onSuccess();
	}
}

// Format : not changes to outputStr
void onDeceleration()
{
	uint8_t target_motor = cmdMessenger.readInt16Arg();
	double deceleration = cmdMessenger.readDoubleArg();

	if (deceleration == 0)
	{
		onFail();
	}
	else
	{
		control.setDeceleration(target_motor, deceleration);
		onSuccess();
	}
}

// Format : not changes to outputStr
void onPower()
{
	uint8_t target_motor = cmdMessenger.readInt16Arg();
	double holdPower = cmdMessenger.readDoubleArg();
	double runPower = cmdMessenger.readDoubleArg();

	if (runPower == 0 || holdPower == 0)
	{
		onFail();
	}
	else
	{
		control.setPower(target_motor, holdPower, runPower);
		onSuccess();
	}
}

// Format : not changes to outputStr
void onDirection()
{
	uint8_t target_motor = cmdMessenger.readInt16Arg();
	double direction = cmdMessenger.readDoubleArg();

	switch ((int)direction)
	{
	case (2):
	{
		control.setDirections(target_motor, true, false);
	}
	break;

	case (3):
	{
		control.setDirections(target_motor, false, true);
	}
	break;

	case (4):
	{
		control.setDirections(target_motor, false, false);
	}
	break;

	default:
	{
		control.setDirections(target_motor, true, true);
	}
	}

	onSuccess();
}

// Format : not changes to outputStr
void onJStoggleCntl()
{
	ToggleJSmtrlControlMode();
	onSuccess();
}

// Format : not changes to outputStr
void onJSDisable()
{
	//motorFlags[0].isJSEnable = false;
	//control.disableJoystick();
	onSuccess();
}

void onJSEnable()
{
	uint8_t enable_disable = cmdMessenger.readInt16Arg();
	motorFlags[0].isJSEnable = bool(enable_disable);
	onSuccess();
}

// Format : not changes to outputStr
void onMotorStop()
{
	uint8_t target_motor = cmdMessenger.readInt16Arg();

	Serial.print("TargetMotor: ");
	Serial.println(target_motor);
	control.stop(target_motor);
	motorFlags[target_motor].isJSEnable = false;
	motorFlags[target_motor].isSeeking = false;
	motorFlags[target_motor].isPositioning = false;
	onSuccess();
}

// Format : not changes to outputStr
void onMotorHome()
{
	uint8_t target_motor = cmdMessenger.readInt16Arg();
	if (_checkFlags(target_motor))
	{
		control.setHome(target_motor);
		onSuccess();
	}
	else
	{
		onFail();
	}
}

// Format : not changes to outputStr
void onSeek()
{
	uint8_t target_motor = cmdMessenger.readInt16Arg();
	_checkJS(target_motor);
	if (motorFlags[target_motor].isPositioning)
	{
		onFail();
	}
	else
	{
		motorFlags[target_motor].direction = cmdMessenger.readBoolArg();
		motorFlags[target_motor].isSeeking = true;
		onSuccess();
	}
}

// Format : not changes to outputStr
void onResolution()
{
	uint8_t target_motor = cmdMessenger.readInt16Arg();
	int resolution = cmdMessenger.readInt16Arg();
	Serial.println(resolution);
	control.setResolution(target_motor, resolution);
	onSuccess();
}

/* =======================================
	Note that on stop switches and on active
	settings are not guaranteed to work if
	called during a movement (ie: constForward,
	homing), but are guaranteed to work in
	the following call.
======================================= */

// Format : not changes to outputStr
void onActiveSettings()
{
	uint8_t target_motor = cmdMessenger.readInt16Arg();
	bool fw = cmdMessenger.readBoolArg();
	bool bw = cmdMessenger.readBoolArg();
	control.switchActiveEnable(target_motor, fw, bw);
	onSuccess();
}

// Format : not changes to outputStr
void onPing()
{
	Serial.println(F("p,PONG;"));
}


