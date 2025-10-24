#include "HWT906.h"

const uint8_t FRAME_HEADER  = 0x55;
const uint8_t FRAMEID_ACCEL = 0x51;
const uint8_t FRAMEID_GYRO  = 0x52;
const uint8_t FRAMEID_EULER = 0x53;
const size_t FRAME_SIZE = 11;

const float ACC_SCALE   = 16.0f / 32768.0f;
const float GYRO_SCALE  = 2000.0f / 32768.0f;
const float ANGLE_SCALE = 180.0f / 32768.0f;



void HWT906_LIB::init() 
{
    acc_        = {};
    gyro_       = {};
    euler_      = {};
    accValid_   = false;
    gyroValid_  = false;
    eulerValid_ = false;
}


bool HWT906_LIB::parseFrame(const uint8_t * buf) 
{
    if (buf[0] != FRAME_HEADER) 
        return false;

    uint8_t checksum = 0;
    for (std::size_t i = 0; i < FRAME_SIZE - 1; ++i) 
        checksum += buf[i];
    if (checksum != buf[FRAME_SIZE - 1]) 
        return false;

    int16_t raw[4];
    for (int i = 0; i < 4; ++i) 
        raw[i] = bufToInt16(&buf[2 + i*2]);

    switch (buf[1]) {
        case FRAMEID_ACCEL:
            acc_.x           = decodeSensor(raw[0], ACC_SCALE);
            acc_.y           = decodeSensor(raw[1], ACC_SCALE);
            acc_.z           = decodeSensor(raw[2], ACC_SCALE);
            acc_.temperature = decodeTemp(raw[3]);
            accValid_        = true;
            break;

        case FRAMEID_GYRO:
            gyro_.x           = decodeSensor(raw[0], GYRO_SCALE);
            gyro_.y           = decodeSensor(raw[1], GYRO_SCALE);
            gyro_.z           = decodeSensor(raw[2], GYRO_SCALE);
            gyro_.temperature = decodeTemp(raw[3]);
            gyroValid_        = true;
            break;

        case FRAMEID_EULER:
            euler_.x           = decodeSensor(raw[0], ANGLE_SCALE);
            euler_.y           = decodeSensor(raw[1], ANGLE_SCALE);
            euler_.z           = decodeSensor(raw[2], ANGLE_SCALE);
            euler_.temperature = decodeTemp(raw[3]);
            eulerValid_        = true;
            break;

        default:
            return false;
    }

    return true;
}

HWT906_LIB::Vector3f HWT906_LIB::HWTgetAccel() const {
    return acc_;
}

HWT906_LIB::Vector3f HWT906_LIB::HWTgetGyro() const {
    return gyro_;
}

HWT906_LIB::Vector3f HWT906_LIB::HWTgetEuler() const {
    return euler_;
}

bool HWT906_LIB::haveFullTriplet() const {
    return accValid_ && gyroValid_ && eulerValid_;
}

float HWT906_LIB::decodeSensor(int16_t raw, float scale) {
    return static_cast<float>(raw) * scale;
}

float HWT906_LIB::decodeTemp(int16_t raw) {
    return static_cast<float>(raw) / 340.0f + 36.25f;
}

int16_t HWT906_LIB::bufToInt16(const uint8_t* b) {
    return static_cast<int16_t>(b[0] | (b[1] << 8));
}
