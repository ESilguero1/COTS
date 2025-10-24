/***********************************************************************************************//**
 * @file       Adafruit_BLE_App.h
 * @details
 * @author		Miguel Silguero
 * @copyright  Copyright (c) 2024-2025, Horizonless Embedded Solutions LLC
 * @date       07.01.2024 (created)
 *
 **************************************************************************************************/
#ifndef HWT906_H
#define HWT906_H

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

class HWT906_LIB 
{
    public:
        struct Vector3f {
            float x{0}, y{0}, z{0}, temperature{0};
        };

        void init();
        bool parseFrame(const uint8_t * buf);

        Vector3f HWTgetAccel() const;
        Vector3f HWTgetGyro()  const;
        Vector3f HWTgetEuler() const;

        bool haveFullTriplet() const;

    private:
        static float  decodeSensor(int16_t raw, float scale);
        static float  decodeTemp(int16_t raw);
        static int16_t bufToInt16(const uint8_t* b);

        Vector3f acc_;
        Vector3f gyro_;
        Vector3f euler_;
        bool accValid_{false};
        bool gyroValid_{false};
        bool eulerValid_{false};
};


#endif
