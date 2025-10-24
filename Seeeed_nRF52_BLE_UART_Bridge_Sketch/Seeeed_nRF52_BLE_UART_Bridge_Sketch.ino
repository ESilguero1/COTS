// XIAO nRF52840 BLE <-> Serial1 bridge with framing and CRC32
//
// Role:
// 1. BLE -> Serial1: Buffers data received from BLE until ';' is seen, then
//    sends the accumulated payload in a framed packet over Serial1.
// 2. Serial1 -> BLE: Parses framed packets [0x55 0xAA][len(2 BE)][payload][crc32(4 LE)].
//    If the CRC is valid, the payload is forwarded to the BLE UART.
//
// Robustness:
// - CRC32 is used for data integrity check (glitch detection).
// - Frame parsing includes a resynchronization mechanism (discarding bytes until markers are found).
// - Length sanity checks are enforced to prevent buffer overflow from corrupted headers.

#include <bluefruit.h>
#include <Arduino.h>

// --- Configuration ---
// Secondary UART settings
#define SERIAL1_BAUD 115200

// Define SERIAL_DEBUG 1 to enable Serial output for debugging
//#define SERIAL_DEBUG 1

// Framing constants
const uint8_t FRAME_MARKER_H = 0x55;
const uint8_t FRAME_MARKER_L = 0xAA;
const size_t CRC_SIZE = 4;
const size_t HEADER_SIZE = 2 + 2; // Marker (2) + Length (2)
const size_t FRAME_OVERHEAD = HEADER_SIZE + CRC_SIZE;

// --- Buffers and State ---
// Serial1 RX Buffer (Max frame size supported)
const size_t SERIAL1_RX_BUF = 512;
uint8_t serial1_buf[SERIAL1_RX_BUF];
size_t serial1_buf_len = 0;
const uint16_t MAX_PAYLOAD_LEN = SERIAL1_RX_BUF - FRAME_OVERHEAD;

// BLE RX buffering until semicolon (packet delimiter)
const size_t BLE_ACCUM_BUF = 512;
uint8_t ble_accum[BLE_ACCUM_BUF];
size_t ble_accum_len = 0;

// BLE UART Service
BLEUart bleuart;

// --- CRC32 Implementation ---
uint32_t crc_table[256];

void crc32_init() {
    const uint32_t poly = 0xEDB88320UL;
    for (uint32_t i = 0; i < 256; ++i) {
        uint32_t c = i;
        for (uint32_t j = 0; j < 8; ++j) {
            if (c & 1) c = poly ^ (c >> 1);
            else c >>= 1;
        }
        crc_table[i] = c;
    }
}

uint32_t crc32_compute(const uint8_t *data, size_t len) {
    uint32_t c = 0xFFFFFFFFUL;
    for (size_t i = 0; i < len; ++i) {
        uint8_t idx = (uint8_t)((c ^ data[i]) & 0xFF);
        c = crc_table[idx] ^ (c >> 8);
    }
    return c ^ 0xFFFFFFFFUL;
}

// --- Helper Functions for Endianness ---

// Write unsigned 16-bit integer as Big Endian
void write_u16_be(uint8_t *dst, uint16_t v) {
    dst[0] = (v >> 8) & 0xFF;
    dst[1] = v & 0xFF;
}

// Write unsigned 32-bit integer as Little Endian
void write_u32_le(uint8_t *dst, uint32_t v) {
    dst[0] = v & 0xFF;
    dst[1] = (v >> 8) & 0xFF;
    dst[2] = (v >> 16) & 0xFF;
    dst[3] = (v >> 24) & 0xFF;
}

// Read unsigned 16-bit integer as Big Endian
uint16_t read_u16_be(const uint8_t *src) {
    return (uint16_t(src[0]) << 8) | uint16_t(src[1]);
}

// Read unsigned 32-bit integer as Little Endian
uint32_t read_u32_le(const uint8_t *src) {
    return uint32_t(src[0]) | (uint32_t(src[1]) << 8) | (uint32_t(src[2]) << 16) | (uint32_t(src[3]) << 24);
}

// --- Communications Functions ---

// Build and send frame containing payload to Serial1
void send_frame_to_serial1(const uint8_t *payload, uint16_t payload_len) {
    // Safety check: ensure the frame fits in the buffer
    if ((size_t)payload_len > MAX_PAYLOAD_LEN) {
        #ifdef SERIAL_DEBUG
        Serial.print("ERROR: BLE payload too large (");
        Serial.print(payload_len);
        Serial.println(" bytes). Dropped.");
        #endif
        return;
    }

    // This buffer is large enough as checked by MAX_PAYLOAD_LEN
    uint8_t frame[SERIAL1_RX_BUF];
    size_t idx = 0;

    // 1. Frame Markers
    frame[idx++] = FRAME_MARKER_H;
    frame[idx++] = FRAME_MARKER_L;
    
    // 2. Payload Length (Big Endian)
    write_u16_be(frame + idx, payload_len); 
    idx += 2;
    
    // 3. Payload Data
    memcpy(frame + idx, payload, payload_len); 
    idx += payload_len;
    
    // 4. CRC32 Checksum (Little Endian)
    uint32_t crc = crc32_compute(payload, payload_len);
    write_u32_le(frame + idx, crc); 
    idx += 4;

    // Send the complete frame
    Serial1.write(frame, idx);
    Serial1.flush();
}

// BLE RX callback: accumulate until semicolon found, then send framed packet to Serial1
void ble_rx_callback(uint16_t conn_hdl) {
    const size_t CHUNK = 128; // Use local const for read chunk size
    uint8_t buf[CHUNK];

    // Read in a loop until no more data
    while (true) {
        int n = bleuart.read(buf, CHUNK);
        if (n <= 0) break;

        // Process n bytes, accumulating until a ';' packet delimiter is found
        for (int i = 0; i < n; ++i) {
            uint8_t received_byte = buf[i];
            
            // Sliding window buffer management: append new byte, dropping oldest if full
            if (ble_accum_len < BLE_ACCUM_BUF) {
                ble_accum[ble_accum_len++] = received_byte;
            } else {
                // Drop the oldest byte (at index 0) to make room for the new one
                memmove(ble_accum, ble_accum + 1, BLE_ACCUM_BUF - 1);
                ble_accum[BLE_ACCUM_BUF - 1] = received_byte;
                // ble_accum_len remains BLE_ACCUM_BUF
            }

            if (received_byte == ';') {
                // Find the first semicolon in the accumulated buffer
                size_t pos = 0;
                while (pos < ble_accum_len && ble_accum[pos] != ';') pos++;

                if (pos < ble_accum_len) {
                    // A complete command is found (including the ';')
                    size_t payload_len = pos + 1;

                    // Send the command as a framed packet over Serial1
                    send_frame_to_serial1(ble_accum, (uint16_t)payload_len);

                    // Consume the transmitted command and shift remaining data
                    size_t remaining = ble_accum_len - payload_len;
                    if (remaining) {
                        memmove(ble_accum, ble_accum + payload_len, remaining);
                    }
                    ble_accum_len = remaining;
                }
            }
        }
    }
}


// Parse frames from serial1_buf and forward payload to BLE UART
void process_serial1_buffer() {
    while (serial1_buf_len >= FRAME_OVERHEAD) {
        // --- 1. Marker Synchronization ---
        if (!(serial1_buf[0] == FRAME_MARKER_H && serial1_buf[1] == FRAME_MARKER_L)) {
            // Marker mismatch: host frame dropped or communication glitch.
            // Discard the leading byte and attempt to resynchronize the stream.
            memmove(serial1_buf, serial1_buf + 1, --serial1_buf_len);
            #ifdef SERIAL_DEBUG
            Serial.println("SYNC ERROR: Discarding byte to find new marker.");
            #endif
            continue;
        }

        // --- 2. Extract Length and Perform Sanity Check ---
        uint16_t payload_len = read_u16_be(serial1_buf + 2);

        if (payload_len == 0 || payload_len > MAX_PAYLOAD_LEN) {
            // Invalid length (0 or excessively large): Frame corruption detected.
            // Discard the marker bytes (0x55 0xAA) and continue to attempt resync.
            memmove(serial1_buf, serial1_buf + 2, serial1_buf_len - 2);
            serial1_buf_len -= 2;
            #ifdef SERIAL_DEBUG
            Serial.print("LEN ERROR: Corrupt length field (");
            Serial.print(payload_len);
            Serial.println("). Discarding marker.");
            #endif
            continue;
        }

        size_t full_frame_len = FRAME_OVERHEAD + payload_len;

        // --- 3. Wait for Full Frame ---
        if (serial1_buf_len < full_frame_len) {
            // Incomplete frame, break and wait for more data in loop()
            break;
        }

        // --- 4. Validate CRC ---
        const uint8_t *payload_ptr = serial1_buf + HEADER_SIZE;
        uint32_t received_crc = read_u32_le(payload_ptr + payload_len);
        uint32_t calc_crc = crc32_compute(payload_ptr, payload_len);

        if (received_crc == calc_crc) {
            // CRC Match: Data is valid, forward the payload
            if (Bluefruit.connected()) {
                bleuart.write(payload_ptr, payload_len);
                #ifdef SERIAL_DEBUG
                //Serial.print("Serial1 -> BLE: Sent valid payload, len=");
                //Serial.println(payload_len);
                #endif
            }

            // Consume the valid frame
            size_t remaining = serial1_buf_len - full_frame_len;
            if (remaining) {
                memmove(serial1_buf, serial1_buf + full_frame_len, remaining);
            }
            serial1_buf_len = remaining;
        } else {
            // CRC Mismatch: Subtle data difference/corruption detected.
            // Discard one byte (the first frame marker) and attempt to resync on the next byte.
            memmove(serial1_buf, serial1_buf + 1, --serial1_buf_len);
            #ifdef SERIAL_DEBUG
            Serial.println("CRC MISMATCH: Frame corrupted. Discarding byte for resync.");
            #endif
        }
    }
}

// --- Main Arduino Functions ---

#ifndef LED_BUILTIN
#define LED_PIN 13
#else
#define LED_PIN LED_BUILTIN
#endif
const unsigned long LED_INTERVAL_MS = 1000;
unsigned long lastLedToggle = 0;
bool ledState = false;

void setup() 
{
    #ifdef SERIAL_DEBUG
    Serial.begin(115200);
    while (!Serial) delay(10);
    Serial.println("Debug Serial started.");
    #endif

    // Setup LED
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
    lastLedToggle = millis();

    // Initialize CRC table
    crc32_init();

    // Initialize Serial1 (Host communication)
    Serial1.begin(SERIAL1_BAUD);

    // Initialize Bluefruit (BLE)
    Bluefruit.configPrphBandwidth(BANDWIDTH_MAX);
    Bluefruit.begin();
    Bluefruit.setName("COTS-BLE-nRF52-Bridge");
    Bluefruit.setTxPower(4);

    // Initialize BLE UART Service and register RX callback
    bleuart.begin();
    bleuart.setRxCallback(ble_rx_callback);

    // Configure and start advertising
    Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
    Bluefruit.Advertising.addTxPower();
    Bluefruit.Advertising.addName();
    Bluefruit.Advertising.addService(bleuart);
    Bluefruit.Advertising.restartOnDisconnect(true);
    Bluefruit.Advertising.setInterval(32, 244); // 20ms to 152.5ms interval
    Bluefruit.Advertising.start(0);

    #ifdef SERIAL_DEBUG
    Serial.println("BLE<->Serial1 bridge started (COTS-BLE-nRF52-Bridge)");
    #endif
}

void loop() {
    // --- Serial1 RX (Host to BLE) ---
    while (Serial1.available()) 
    {
        int c = Serial1.read();
        if (c < 0) break;

        // Store incoming byte in the serial1_buf
        if (serial1_buf_len < SERIAL1_RX_BUF) 
        {
            serial1_buf[serial1_buf_len++] = (uint8_t)c;
        } 
        else 
        {
            // Overflow: Drop oldest byte (sliding window)
            memmove(serial1_buf, serial1_buf + 1, SERIAL1_RX_BUF - 1);
            serial1_buf[SERIAL1_RX_BUF - 1] = (uint8_t)c;
            #ifdef SERIAL_DEBUG
            Serial.println("Serial1 RX Buffer Overflow (sliding oldest byte)");
            #endif
        }
    }

    // Process all accumulated Serial1 data (attempts to find and forward frames)
    if (serial1_buf_len > 0) 
    {
        process_serial1_buffer();
    }

    // Small delay to yield CPU time to the BLE stack
    delay(1);
    
    // --- Status LED Blink ---
    unsigned long now = millis();
    if ((now - lastLedToggle) >= LED_INTERVAL_MS) 
    {
        lastLedToggle = now;
        ledState = !ledState;
        digitalWrite(LED_PIN, ledState ? HIGH : LOW);
    }
}