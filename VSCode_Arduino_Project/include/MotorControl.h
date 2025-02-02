#ifndef MotorControl_H

/* ========================================================================
   $File: MotorControl.h$
   $Date: $
   $Revision: $
   $Creator:  $
   $Email:  $
   $Notice: $
   ======================================================================== */

#define MotorControl_H
#include "System_definitions.h"
#include <SPI.h>
#include "Arduino.h"
#include "Joystick.h"

// Debug options
// #define MOTOR_DEBUG 1


// Define Registers for TMC5130
#define ADDRESS_GCONF      	0x00
#define ADDRESS_GSTAT      	0x01
#define ADDRESS_IFCNT      	0x02
#define ADDRESS_SLAVECONF  	0x03
#define ADDRESS_INP_OUT    	0x04
#define ADDRESS_X_COMPARE  	0x05
#define ADDRESS_IHOLD_IRUN 	0x10
#define ADDRESS_TZEROWAIT  	0x11
#define ADDRESS_TSTEP  		0x12
#define ADDRESS_TPWMTHRS  	0x13
#define ADDRESS_TCOOLTHRS  	0x14
#define ADDRESS_THIGH      	0x15

#define ADDRESS_RAMPMODE   	0x20
#define ADDRESS_XACTUAL    	0x21
#define ADDRESS_VACTUAL    	0x22
#define ADDRESS_VSTART     	0x23
#define ADDRESS_A1         	0x24
#define ADDRESS_V1         	0x25
#define ADDRESS_AMAX       	0x26
#define ADDRESS_VMAX       	0x27
#define ADDRESS_DMAX       	0x28
#define ADDRESS_D1         	0x2A
#define ADDRESS_VSTOP      	0x2B
#define ADDRESS_TZEROCROSS 	0x2C
#define ADDRESS_XTARGET    	0x2D

#define ADDRESS_VDCMIN     	0x33
#define ADDRESS_SWMODE     	0x34
#define ADDRESS_RAMPSTAT   	0x35
#define ADDRESS_XLATCH     	0x36
#define ADDRESS_ENCMODE    	0x38
#define ADDRESS_XENC       	0x39
#define ADDRESS_ENC_CONST  	0x3A
#define ADDRESS_ENC_STATUS 	0x3B
#define ADDRESS_ENC_LATCH  	0x3C

#define ADDRESS_MSLUT0     	0x60
#define ADDRESS_MSLUT1     	0x61
#define ADDRESS_MSLUT2     	0x62
#define ADDRESS_MSLUT3     	0x63
#define ADDRESS_MSLUT4     	0x64
#define ADDRESS_MSLUT5     	0x65
#define ADDRESS_MSLUT6     	0x66
#define ADDRESS_MSLUT7     	0x67
#define ADDRESS_MSLUTSEL   	0x68
#define ADDRESS_MSLUTSTART 	0x69
#define ADDRESS_MSCNT      	0x6A
#define ADDRESS_MSCURACT   	0x6B
#define ADDRESS_CHOPCONF   	0x6C
#define ADDRESS_COOLCONF   	0x6D
#define ADDRESS_DCCTRL     	0x6E
#define ADDRESS_DRVSTATUS  	0x6F
#define ADDRESS_PWMCONF  	0x70
#define ADDRESS_PWMSTATUS 	0x71
#define ADDRESS_EN_CTRL 	0x72
#define ADDRESS_LOST_STEPS 	0x73

// Register ADDRESS_RAMPMODE
#define ADDRESS_MODE_POSITION   0
#define ADDRESS_MODE_VELPOS     1
#define ADDRESS_MODE_VELNEG     2
#define ADDRESS_MODE_HOLD       3

// Register ADDRESS_SWMODE
#define ADDRESS_SW_STOPL_ENABLE   0x0001
#define ADDRESS_SW_STOPR_ENABLE   0x0002
#define ADDRESS_SW STOPL_POLARITY 0x0004
#define ADDRESS_SW_STOPR_POLARITY 0x0008
#define ADDRESS_SW_SWAP_LR        0x0010
#define ADDRESS_SW_LATCH_L_ACT    0x0020
#define ADDRESS_SW_LATCH_L_INACT  0x0040
#define ADDRESS_SW_LATCH_R_ACT    0x0080
#define ADDRESS_SW_LATCH_R_INACT  0x0100
#define ADDRESS_SW_LATCH_ENC      0x0200
#define ADDRESS_SW_SG_STOP        0x0400
#define ADDRESS_SW_SOFTSTOP       0x0800


// Register ADDRESS_RAMPSTAT
#define ADDRESS_RS_STOPL          0x0001
#define ADDRESS_RS_STOPR          0x0002
#define ADDRESS_RS_LATCHL         0x0004
#define ADDRESS_RS_LATCHR         0x0008
#define ADDRESS_RS_EV_STOPL       0x0010
#define ADDRESS_RS_EV_STOPR       0x0020
#define ADDRESS_RS_EV_STOP_SG     0x0040
#define ADDRESS_RS_EV_POSREACHED  0x0080
#define ADDRESS_RS_VELREACHED     0x0100
#define ADDRESS_RS_POSREACHED     0x0200
#define ADDRESS_RS_VZERO          0x0400
#define ADDRESS_RS_ZEROWAIT       0x0800
#define ADDRESS_RS_SECONDMOVE     0x1000
#define ADDRESS_RS_SG             0x2000


struct datagram {
	public:
		byte rw = 0x0;
		byte address = 0x0;
		unsigned long data = 0x0;
		byte responseFlags = 0x0;
};

struct directionControl {
	public: 
		int activeEnableNum;
		int buttonStatusNum;
		int dirMultiplier;

		unsigned long address;
};

class MotorControl {

	public:

		// Class functions
		MotorControl(byte csPin, byte enablePin, int ID); //Constructor
		MotorControl();

		void set(byte csPin, byte enablePin, int ID);
		void begin(); //initialise motor settings

		// Settings
		bool IsPositionMode;
		bool IsForward;
		int motorID;

		// Build direction storing objects
		directionControl forwardDirection;
		directionControl backwardDirection;

		// Motor datagrams to send in for 
		// read and write direction
		datagram GCONF;
		datagram IHOLD_IRUN;
		datagram TPOWERDOWN;
		datagram CHOPCONF;
		datagram RAMPMODE;
		datagram PWMCONF;

		datagram XACTUAL;
		datagram XTARGET;
		datagram HOME_XTARGET;

		datagram A1;
		datagram V1;
		datagram D1;
		datagram AMAX;
		datagram VMAX;
		datagram DMAX;
		datagram VSTOP;
		datagram SW_MODE;

		datagram XACTUAL_READ;
		datagram DRV_STATUS_READ;
		datagram GCONF_READ;
		datagram GSTAT_READ;
		datagram RAMP_STAT_READ;

	    // SPI Reads
	    datagram i_datagram;

	    // Status Bits read in from registers
	    // to check condition of the motor
	    bool status_sg2;
	    bool status_sg2_event;
	    bool status_standstill;

	    bool status_velocity_reached;
	    bool status_position_reached;
	    bool status_position_reached_event;

	    bool status_stop_l;
	    bool status_stop_r;
	    bool status_stop_l_event;
	    bool status_stop_r_event;
	    bool status_latch_l;
	    bool status_latch_r;

	    bool status_openLoad_A;
	    bool status_openLoad_B;
	    bool status_shortToGround_A;
	    bool status_shortToGround_B;

	    bool status_overtemperatureWarning;
	    bool status_overtemperatureShutdown;

	    bool status_isReverse;
	    bool status_resetDetected;
	    bool status_driverError;
	    bool status_underVoltage;

	    unsigned long sgStatusBits;

	    bool forwardSwitch;
	    bool backwardSwitch;


		//======================= HELPER FUNCTIONS =====================

		void sendData(datagram * datagram);
		void getMotorData();
		void readStatus();
		void sgStatus();
		void buttonStatus();
		void switchReference(bool rightIsREFR);


		//================= ENABLE AND DISABLE FUNCTIONS ===============

		bool csEnable(); 								// enable chip select pin, call before sending datagram
		bool csDisable(); 								// disable chip select pin, latch
		bool powerEnable(); 							// enable power to this motor
		bool powerDisable(); 							// disable power to this motor
		bool switchActiveEnable(bool fw, bool bw);		// enables active high or low for the switches


		//=================== READ AND WRITE FUNCTIONS ================

		void setPowerLevel(unsigned long holdPower, unsigned long runPower); 	//32bit binary
		void setVelocity(unsigned long velocity); 								//32bit binary
		void setAcceleration(unsigned long acceleration); 						//32bit binary
		void setDeceleration(unsigned long acceleration); 						//32bit binary
		void setXtarget(unsigned long xtarget); 								//32bit binary
		void setXactual(unsigned long xactual); 								//32bit binary
		void setRampMode(unsigned long rampMode); 								//32bit binary
		void setChopConf(int resolution);

		int getMotorID();
		int getResolution();
		unsigned long getPowerLevel();
		unsigned long getVelocity();
		unsigned long getAcceleration();
		unsigned long getDeceleration();
		signed long getXtarget();
		unsigned long getXactual();
		unsigned long getHomeXtarget();
		unsigned long getRampMode();
		bool getIsForward();
		bool getIsPositionMode();
		bool getIsHomed();


		//===================== MOVEMENT FUNCTIONS ==================

		bool goPos(unsigned long position); 				// brings the motor back to its home position
		bool setHome(); 									// sets the home position using the right hand switch
		bool stop();
		// bool movement(bool direction, bool type, unsigned long speed, unsigned long steps);
		void swapDirection(bool swapDirection, bool swapSwitch);
		void constForward(unsigned long velocity);							// moves the motor forward constantly
		void constReverse(unsigned long velocity);							// moves the motor backwards constantly
		void forward(unsigned long stepsForward, unsigned long velocity); 	// push forward at the specified velocity
		void reverse(unsigned long stepsBackward, unsigned long velocity);	// moves motor in reverse direction

	private:

		int _csPin;
		int _enablePin;
		int _microSteps;
		int _homeCaseNum;
		int _resolutionNum;

		datagram _outputDatagram;

		bool _isHomed;
		bool _checkBit(datagram * dg, int targetBit);
};

#endif