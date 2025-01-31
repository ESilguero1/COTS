#include "Adafruit_BLE_App.h"
#include "System_definitions.h"
#include "CmdMessenger.h"

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

extern CmdMessenger cmdMessenger;

String BLE_Str;
//BLE instatiation
Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_CS, BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);
//BLE #instatiation


// =============== Arduino Functions and Calls ===============

uint8_t Adafruit_BLE_App :: Init()
{
	uint8_t	initState = 1;

	//Configure BLE pins
	pinMode(BLUEFRUIT_SPI_CS, OUTPUT);
	pinMode(BLUEFRUIT_SPI_RST, OUTPUT);
	digitalWrite(BLUEFRUIT_SPI_RST, HIGH);

	// =============== Initialize BLE String Objects ===============

	BLE_Str.reserve(BLE_CHARS_SIZE);

	/* Initialise the module */
	Serial.print(F("Initialising the Bluefruit LE module: "));

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
		delay(1000);
		if (ble.isConnected()) 
		{
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
			Serial.println("BLE INIT completed successfully");
			initState = 0;
		}
		else
		{
			Serial.println("BLE INIT FAILED");
			Serial.println(F("Please use Adafruit Bluefruit LE app to connect in UART mode"));
			Serial.println(F("Then Enter characters to send to Bluefruit"));
			Serial.println();
		}

	} 
	
	return initState;
}


void Adafruit_BLE_App :: Service_BLE_UART()
{
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