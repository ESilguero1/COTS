/***********************************************************************************************//**
 * @file       System_Control_App.h
 * @details 
 * @author		Miguel Silguero
 * @copyright  Copyright (c) 2024-2025, Horizonless Embedded Solutions LLC
 * @date       12.01.2024 (created)
 *
 **************************************************************************************************/
#ifndef System_Control_App_H
#define System_Control_App_H

/***************************************************************************************************
 * INCLUDES
 **************************************************************************************************/
#include "Arduino.h"

/***************************************************************************************************
 * CONSTANTS AND DEFINITIONS
 **************************************************************************************************/
#define JS_SWITCH_CHK (200)
/***************************************************************************************************
 * PUBLIC FUNCTION PROTOTYPES
 **************************************************************************************************/

class System_Control_App 
{
      public:
            void Init(void);
            void ServiceSystemResponseApp(void);
            void ServiceMotor3PowerDisable(void);
            uint32_t RequestMotorStatus(uint8_t target_motor);
            void ToggleJSmtrlControlMode(void);
            void SetSysInitstate(uint8_t state);
};

#endif
