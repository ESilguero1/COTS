#include "CmdMessenger.h"
#include "System_definitions.h"
#include "Enum.h"
#include "debugutils.h"
#include "CombinedControl.h"

///BLE imports
#include <Arduino.h>
#include <SPI.h>
#include "Adafruit_BLE.h"
#include "Adafruit_BluefruitLE_SPI.h"
#include "Adafruit_BluefruitLE_UART.h"

#include "BluefruitConfig.h"
//BLE #defines
    #define FACTORYRESET_ENABLE         0
    #define MINIMUM_FIRMWARE_VERSION    "0.6.6"
    #define MODE_LED_BEHAVIOUR          "MODE"
//BLE #defines

//#define DEBUG 1
//#define DEBUG_CO 1
#define BLE_CHARS_SIZE 64

// Build extra objects
CmdMessenger cmdMessenger = CmdMessenger(Serial);
String outputStr;
CombinedControl control;
flags motorFlags[2];
int status[25];

String BLE_Str;
//BLE instatiation
Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_CS, BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);
//BLE #instatiation

void attachCommandCallbacks();
void _checkJS(uint8_t motorID);
void OnUnknownCommand();
void onRequestMotorStatus();
void onRequestStallStatus();
void onRequestSetPosNoMove();
void onSetSlowFastJSMotion();
void onSetJSMirrorMode();
void onGetXactual();
void onGetVelocity();
void onGetAcceleration();
void onGetDeceleration();
void onGetPower();
void onConstantForward();
void onConstantBackward();
void onMovePosition();
void onMoveForward();
void onMoveBackward();
void onVelocity();
void onAcceleration();
void onDeceleration();
void onPower();
void onDirection();
void onJSEnable();
void onJSDisable();
void onMotorStop();
void onMotorHome();
void onSeek();
void onResolution();
void onActiveSettings();
void onPing();

// =============== Arduino Functions and Calls ===============

void setup()
{

	// =============== Setup Motor Control ===============

	// Configure Motor enable and Chip Select Pins
	pinMode(MTR_CS0, OUTPUT);
	pinMode(MTR_CS1, OUTPUT);
	pinMode(MTR_ENA_0, OUTPUT);
	pinMode(MTR_ENA_1, OUTPUT);

	// Configure joystick stop switch
	pinMode(JS_STOP_SWTICH, INPUT);


	//Configure BLE pins
	pinMode(BLUEFRUIT_SPI_CS, OUTPUT);
	pinMode(BLUEFRUIT_SPI_RST, OUTPUT);
	digitalWrite(BLUEFRUIT_SPI_RST, HIGH);

	// initialize the SPI interface with TMC5130 settings
	SPI.setBitOrder(MSBFIRST);
	SPI.setClockDivider(SPI_CLOCK_DIV8);
	SPI.setDataMode(SPI_MODE3);
	SPI.begin();

	// =============== Setup Board Configuration and Sensors ===============

	Serial.begin(115200);
	Serial.println("starting the coolest project in the history of mankind");

	// =============== Initialize Objects ===============

	// Initialize the motor and Sensor objects
	control.begin();

	outputStr.reserve(128);
	BLE_Str.reserve(BLE_CHARS_SIZE);
	cmdMessenger.printLfCr();
	attachCommandCallbacks();



	/* Initialise the module */
	Serial.print(F("Initialising the Bluefruit LE module: "));
/* 
	if ( !ble.begin(VERBOSE_MODE) )
	{
		Serial.print(F("Couldn't find Bluefruit, make sure it's in CoMmanD mode & check wiring?"));
	}
	else
	{
		Serial.println( F("BLE OK!") );
		// Disable command echo from Bluefruit //
		ble.echo(false);

		Serial.println("Requesting Bluefruit info:");
		// Print Bluefruit information //
		ble.info();

		Serial.println(F("Please use Adafruit Bluefruit LE app to connect in UART mode"));
		Serial.println(F("Then Enter characters to send to Bluefruit"));
		Serial.println();

		ble.verbose(false);  // debug info is a little annoying after this point!
		// Wait for connection //
		while (! ble.isConnected()) 
		{
			delay(200);
			if (digitalRead(JS_STOP_SWTICH) == LOW)
			{
				break;
			}
		}

		// LED Activity command is only supported from 0.6.6
		if ( ble.isVersionAtLeast(MINIMUM_FIRMWARE_VERSION) )
		{
			// Change Mode LED Activity
			Serial.println(F("Change LED activity to " MODE_LED_BEHAVIOUR));
			ble.sendCommandCheckOK("AT+HWModeLED=" MODE_LED_BEHAVIOUR);
		}

		// Set module to DATA mode
		Serial.println( F("Switching to DATA mode!") );
		ble.setMode(BLUEFRUIT_MODE_DATA);
		ble.print("BLE INIT completed");
		Serial.println("BLE INIT completed");
	} 
	*/
	
}

void loop()
{

	// 20 us
	cmdMessenger.feedinSerialData(); /* Process incoming messages */

	// 100 us
	if (motorFlags[0].isJSEnable)
	{
		control.enableJoystick();
	}

	for (uint8_t motor = 0; motor < 2; motor++)
	{
		// 12 us
		if (motorFlags[motor].isSeeking)
		{
			motorFlags[motor].isSeeking = !control.seek(motor, motorFlags[motor].direction);
		}

		// 50 us
		if (motorFlags[motor].isPositioning)
		{
			motorFlags[motor].isPositioning = !control.standstill(motor);
		}
	}

	// Process BLE received characters
	if ( ble.available() )
	{
		char lastBLEcharReceived = (char)(ble.read());
		BLE_Str.concat(lastBLEcharReceived);
		if (lastBLEcharReceived == ';')
		{

			for (uint8_t e = 0; e < BLE_CHARS_SIZE; e++)
			{
				int messageState = cmdMessenger.processLine(BLE_Str[e]);

				// If waiting for acknowledge command
				if (messageState == kEndOfMessage)
				{
					cmdMessenger.handleMessage();
					break;
				}
			}
			BLE_Str.remove(0);
		}
	}

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

	cmdMessenger.attach(JS_ENABLE, onJSEnable);			   // Reply: S,1;
	cmdMessenger.attach(JS_DISABLE, onJSDisable);		   // Reply: S,1;
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
}

void onFail()
{
	outputStr.remove(0);
	outputStr.concat(F("S,0;"));
	Serial.println(outputStr);
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

	outputStr.concat(F(";"));
	Serial.println(outputStr);
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
void onJSEnable()
{
	motorFlags[0].isJSEnable = true;
	control.enableJoystick();
	onSuccess();
}

// Format : not changes to outputStr
void onJSDisable()
{
	motorFlags[0].isJSEnable = false;
	control.disableJoystick();
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
