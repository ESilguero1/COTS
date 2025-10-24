/***********************************************************************************************//**
 * @file       blebridge.h
 * @details
 * @author		Miguel Silguero
 * @copyright  Copyright (c) 2024-2025, Horizonless Embedded Solutions LLC
 * @date       07.01.2024 (created)
 *
 **************************************************************************************************/
#ifndef BLEBRIDGE_H
#define BLEBRIDGE_H

/***************************************************************************************************
 * INCLUDES
 **************************************************************************************************/
#include "Arduino.h"

/***************************************************************************************************
 * TYPEDEFS
 **************************************************************************************************/

/***************************************************************************************************
 * PUBLIC FUNCTION PROTOTYPES
 **************************************************************************************************/

class BLEBRIDGE_LIB 
{
    public:
		// Build a frame into dst. Returns frame length in bytes, or 0 if dst_len too small.

		size_t build_frame_from_cstr(const char *payload_cstr, uint8_t *dst, size_t dst_len);


		// Parse one frame from buf (buf_len bytes available).
		// On success: returns payload_len (>0), writes pointer to payload_offset and consumed bytes via consumed_out.
		// On failure: returns 0; consumed_out indicates how many leading bytes to drop for resync (usually 1).
		size_t parse_frame(const uint8_t *buf, size_t buf_len,
                   size_t *payload_offset_out, uint16_t *payload_len_out, size_t *consumed_out);
				// Compute CRC32 (IEEE 802.3)
		uint32_t computeCRC(const uint8_t *data, size_t len); 

};


#endif
