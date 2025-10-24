/***********************************************************************************************//**
 * @file       BLE_Bridge_App.cpp
 * @details    Bridge application to handle BLE communication using attached Seeedstudio BLE module.
 * @author		Miguel Silguero
 * @copyright  Copyright (c) 2024-2025, Horizonless Embedded Solutions LLC
 * @date       07.01.2024 (created)
 *
 **************************************************************************************************/

/***************************************************************************************************
 * INCLUDES
 * 
 * Include necessary libraries for serial comm with BLE module.
 **************************************************************************************************/
#include "BLE_Bridge_App.h"
#include "blebridge.h"
#include <Arduino.h>
#include "CmdMessenger.h"
#include "System_definitions.h"

/***************************************************************************************************
 * CONSTANTS AND DEFINITIONS
 * 
 * Define constants for BLE settings, firmware version, and buffer sizes.
 **************************************************************************************************/

#define LINK_SERIAL     Serial3    // Serial2 used to talk to XIAO (TX2/RX2)
#define LINK_BAUD       115200

const size_t MAX_FRAME_BUF = 256;        // accumulator and tx buffers

// ----- Buffers/state -----
uint8_t tx_frame_buf[MAX_FRAME_BUF];
uint8_t rx_accum_buf[MAX_FRAME_BUF];
size_t rx_accum_len = 0;

void handle_received_frame(const uint8_t *frame_ptr, size_t frame_len);
void parse_frames_from_BLE();

/***************************************************************************************************
 * MODULE VARIABLES
 * 
 * Declare global and external variables used in BLE communication.
 **************************************************************************************************/
BLEBRIDGE_LIB BLE_Bridge_Lib;
extern CmdMessenger cmdMessenger; /* External command instance for servicing BLE communication */
/***************************************************************************************************
 * FUNCTION DEFINITIONS
 **************************************************************************************************/


/***********************************************************************************************//**
 * @details     Initializes the BLE module and establishes a connection.
 * @return      Returns 0 on successful initialization, 1 on failure.
 **************************************************************************************************/
uint8_t BLE_Bridge_App :: Init()
{
	uint8_t initState = 1; /* Initialize state variable, 1 indicates failure by default */

  // Link serial (Serial2)
  pinMode(PIN_TX2, OUTPUT);
  pinMode(PIN_RX2, INPUT);
  LINK_SERIAL.begin(LINK_BAUD);
  delay(50);

  // init state
  rx_accum_len = 0;

    size_t frame_len = BLE_Bridge_Lib.build_frame_from_cstr("Hello Mundo", tx_frame_buf, sizeof(tx_frame_buf));
    if (frame_len == 0) 
    {
      Serial.println("Error: build_frame failed (buffer too small)");
    } else 
    {
      // send to XIAO via Serial2
      LINK_SERIAL.write(tx_frame_buf, frame_len);
      LINK_SERIAL.flush();
    }


	return initState; /* Return the initialization state */
}

/***********************************************************************************************//**
 * @details     Handles incoming BLE UART data and processes commands.
 **************************************************************************************************/
void BLE_Bridge_App :: Service_BLE_UART()
{
  // Read incoming bytes from Seeed nRF52 BLE on Serial2 into accumulator
  while (LINK_SERIAL.available())
  {
    int b = LINK_SERIAL.read();
    if (b < 0) break;
    if (rx_accum_len < MAX_FRAME_BUF) 
    {
      rx_accum_buf[rx_accum_len++] = (uint8_t)b;
    } 
    else 
    {
      // overflow: drop oldest byte to make room
      memmove(rx_accum_buf, rx_accum_buf + 1, MAX_FRAME_BUF - 1);
      rx_accum_buf[MAX_FRAME_BUF - 1] = (uint8_t)b;
    }
  }

  // Try to parse and handle frames
  if (rx_accum_len > 0) 
  {
    parse_frames_from_BLE(); /* Validate and service frame from Seeed nRF52 BLE*/
  }
}

void BLE_Bridge_App ::println(const String &s)
{
    size_t frame_len = BLE_Bridge_Lib.build_frame_from_cstr(s.c_str(), tx_frame_buf, sizeof(tx_frame_buf));
    if (frame_len == 0) 
    {
      Serial.println("Error: build_frame failed (buffer too small)");
    } else 
    {
      // send to XIAO via Serial2
      LINK_SERIAL.write(tx_frame_buf, frame_len);
      LINK_SERIAL.flush();
    }

}


// Validate and handle a complete frame starting at frame_ptr with full length frame_len
// parse_frame() already validates CRC in typical implementations, but we re-validate here
void handle_received_frame(const uint8_t *frame_ptr, size_t frame_len) {
  if (frame_len < 8) return; // minimum frame size
  if (!(frame_ptr[0] == 0x55 && frame_ptr[1] == 0xAA)) return; // marker check

  uint16_t payload_len = (uint16_t(frame_ptr[2]) << 8) | uint16_t(frame_ptr[3]);
  size_t expected_full = 2 + 2 + payload_len + 4;
  if (frame_len < expected_full) return;

  const uint8_t *payload_ptr = frame_ptr + 4;
  const uint8_t *crc_ptr = payload_ptr + payload_len;

  uint32_t recv_crc = (uint32_t)crc_ptr[0]
                    | ((uint32_t)crc_ptr[1] << 8)
                    | ((uint32_t)crc_ptr[2] << 16)
                    | ((uint32_t)crc_ptr[3] << 24);

  uint32_t calc_crc = BLE_Bridge_Lib.computeCRC(payload_ptr, payload_len);

  if (recv_crc != calc_crc) {
    Serial.print("Frame CRC mismatch: recv=0x");
    Serial.print(recv_crc, HEX);
    Serial.print(" calc=0x");
    Serial.println(calc_crc, HEX);
    return;
  }

  // CRC valid: print and optionally act on payload
  //Serial.print("Valid payload len=");
  //Serial.print(payload_len);
  //Serial.print(" : ");
  for (uint16_t i = 0; i < payload_len; ++i) 
  {
    char c = (char)payload_ptr[i]; // cast byte to a character for printable check
    if (c >= 32 && c <= 126) // test if character is printable ASCII range
    {
		Serial.print(c); // print the printable character to debug serial
		int messageState = cmdMessenger.processLine(c);

		/* Check if message processing is complete */
		if (messageState == kEndOfMessage)
		{
			cmdMessenger.handleMessage();
			break;
		}  
    }
    else 
    {
        Serial.print('.');
    }
  }

}

// Attempt to parse frames from rx_accum_buf using parse_frame() from framing_api.h
// parse_frame signature:
//   size_t parse_frame(const uint8_t *buf, size_t buf_len,
//                      size_t *payload_offset_out, uint16_t *payload_len_out, size_t *consumed_out);
// Returns payload_len on success, 0 on failure or incomplete; consumed_out set accordingly.
void parse_frames_from_BLE() 
{
  while (true) {
    if (rx_accum_len < 8) return; // minimum frame not available
    size_t payload_offset = 0;
    uint16_t payload_len = 0;
    size_t consumed = 0;
    size_t parsed = BLE_Bridge_Lib.parse_frame(rx_accum_buf, rx_accum_len, &payload_offset, &payload_len, &consumed);

    if (consumed == 0) {
      
      return;
    }

    if (parsed == 0) {
      // parse failed (resync); drop 'consumed' bytes and continue
      if (consumed >= rx_accum_len) {
        rx_accum_len = 0;
      } else {
        memmove(rx_accum_buf, rx_accum_buf + consumed, rx_accum_len - consumed);
        rx_accum_len -= consumed;
      }
      continue;
    }

    // Success: full valid frame consumed bytes = consumed; payload_offset is offset within frame where payload starts
    // Pass the entire frame to handler for CRC double-check and action
    handle_received_frame(rx_accum_buf, consumed);

    // Remove consumed bytes from accumulator
    if (consumed >= rx_accum_len) {
      rx_accum_len = 0;
      return;
    } else {
      memmove(rx_accum_buf, rx_accum_buf + consumed, rx_accum_len - consumed);
      rx_accum_len -= consumed;
      // continue to parse additional frames if present
    }
  }
}
