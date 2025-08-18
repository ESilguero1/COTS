/***********************************************************************************************//**
 * @file       Adafruit_BLE_App.cpp
 * @details    IMU application module. Services IMU initialization, data collection and filtering
 * @author		Miguel Silguero
 * @copyright  Copyright (c) 2024-2025, Horizonless Embedded Solutions LLC
 * @date       07.01.2024 (created)
 *
 **************************************************************************************************/

/***************************************************************************************************
 * INCLUDES
 * 
 * Include necessary libraries for BLE communication, system definitions, and Arduino SPI support.
 **************************************************************************************************/
#include "Adafruit_BLE_App.h"
#include "System_definitions.h"
#include "CmdMessenger.h"
#include <Arduino.h>
#include <SPI.h>
#include "Adafruit_BLE.h"
#include "Adafruit_BluefruitLE_SPI.h"
#include "Adafruit_BluefruitLE_UART.h"
#include "BluefruitConfig.h"

/***************************************************************************************************
 * CONSTANTS AND DEFINITIONS
 * 
 * Define constants for BLE settings, firmware version, and buffer sizes.
 **************************************************************************************************/
#define FACTORYRESET_ENABLE         0      /* Enable or disable factory reset during initialization */
#define MINIMUM_FIRMWARE_VERSION    "0.6.6" /* Minimum required firmware version for BLE module */
#define MODE_LED_BEHAVIOUR          "MODE"  /* Define LED behavior mode for BLE module */
//#define DEBUG 1                        /* Uncomment to enable debugging */
//#define DEBUG_CO 1                     /* Uncomment to enable CO debugging */
#define BLE_CHARS_SIZE 64              /* Define the buffer size for BLE communication */

/***************************************************************************************************
 * MODULE VARIABLES
 * 
 * Declare global and external variables used in BLE communication.
 **************************************************************************************************/
extern CmdMessenger cmdMessenger; /* External command messenger instance for BLE communication */
String BLE_Str;                   /* String buffer for storing received BLE data */

/* BLE module instantiation with SPI interface */
Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_CS, BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);

/***************************************************************************************************
 * FUNCTION DEFINITIONS
 **************************************************************************************************/

/***********************************************************************************************//**
 * @details     Initializes the BLE module and establishes a connection.
 * @return      Returns 0 on successful initialization, 1 on failure.
 **************************************************************************************************/
uint8_t Adafruit_BLE_App :: Init()
{
	uint8_t initState = 1; /* Initialize state variable, 1 indicates failure by default */

	/* Configure BLE pins as output */
	pinMode(BLUEFRUIT_SPI_CS, OUTPUT);
	pinMode(BLUEFRUIT_SPI_RST, OUTPUT);
	//digitalWrite(BLUEFRUIT_SPI_RST, LOW);
	//delay(250);
	digitalWrite(BLUEFRUIT_SPI_RST, HIGH);
	//delay(250);
	
	/* Reserve space for BLE string buffer */
	BLE_Str.reserve(BLE_CHARS_SIZE);

	/* Initialize BLE module */
	//.print(F("Init the Bluefruit LE "));

	if (!ble.begin(VERBOSE_MODE))
	{
		/* Print error message if BLE module is not found */
		Serial.print(F("Couldn't find Bluefruit, make sure it's in CoMmanD mode & check wiring?"));
	}
	else
	{
		/* Print success message and disable command echo */
		//Serial.println(F("BLE OK!"));
		//ble.factoryReset();
		ble.echo(false);

		/* Print BLE module information */
		//ble.info();
		//Serial.println();

		/* Disable verbose debugging mode */
		//ble.verbose(false);

		/* Wait for BLE connection */
		uint8_t delayms = 250;
		uint8_t delaysecs = 20;
		uint8_t iterations = delaysecs*(1.0/(float(delayms)/1000.0));
		for (uint8_t dls = 0; dls < iterations; dls++)
		{
			if (ble.isConnected())
			{
				Serial.println(F("BLE UART OK!"));
				break;
			}
			delay(delayms);
			Serial.println(F("."));
			//digitalWrite(SYS_LED, !digitalRead(SYS_LED));
		}

		if (ble.isConnected())
		{
			delay(100);
			/* Configure LED activity mode if firmware version is sufficient */
			if (ble.isVersionAtLeast(MINIMUM_FIRMWARE_VERSION))
			{
				//Serial.println(F("Change LED activity to " MODE_LED_BEHAVIOUR));
				ble.sendCommandCheckOK("AT+HWModeLED=" MODE_LED_BEHAVIOUR);
			}
			delay(100);

			/* Switch BLE module to data mode */
			Serial.println(F("Switching to DATA mode!"));
			ble.setMode(BLUEFRUIT_MODE_DATA);
			//ble.print("BLE INIT completed");
			Serial.println("BLE INIT completed successfully");
			initState = 0; /* Set state to success */
		}
		else
		{
			/* Print failure message if connection fails */
			ble.verbose(true);
			Serial.println("BLE INIT FAILED");
			Serial.println(F("Please use Adafruit Bluefruit LE app to connect in UART mode"));
			Serial.println(F("Then Enter characters to send to Bluefruit"));
			Serial.println();
		}
	}

	return initState; /* Return the initialization state */
}

/***********************************************************************************************//**
 * @details     Handles incoming BLE UART data and processes commands.
 **************************************************************************************************/
void Adafruit_BLE_App :: Service_BLE_UART()
{
	/* Check if there is available data from BLE module */
	if (ble.available())
	{
		char lastBLEcharReceived = (char)(ble.read()); /* Read the received character */
		BLE_Str.concat(lastBLEcharReceived); /* Append to BLE string buffer */

		/* Process BLE command when end-of-command character ';' is received */
		if (lastBLEcharReceived == ';')
		{
			/* Process the received BLE command */
			for (uint8_t e = 0; e < BLE_CHARS_SIZE; e++)
			{
				int messageState = cmdMessenger.processLine(BLE_Str[e]);

				/* Check if message processing is complete */
				if (messageState == kEndOfMessage)
				{
					cmdMessenger.handleMessage();
					break;
				}  
			}

			/* Clear BLE string buffer after processing */
			BLE_Str.remove(0);
		}
	}
}
