/***********************************************************************************************//**
 * @file       HWT906_App.cpp
 * @details    IMU application module. Services IMU initialization, data collection and filtering
 * @author		Miguel Silguero
 * @copyright  Copyright (c) 2024-2025, Horizonless Embedded Solutions LLC
 * @date       09.21.2025 (created)
 *
 **************************************************************************************************/

/***************************************************************************************************
 * INCLUDES
 **************************************************************************************************/
#include "HWT906_App.h"
#include "HWT906.h"
#include "System_definitions.h"
#include <Arduino.h>
//#include <wiring_private.h>
//#include <sam.h>

/***************************************************************************************************
 * CONSTANTS AND DEFINITIONS
 **************************************************************************************************/
static constexpr size_t PACKET_LEN = 33;

/***************************************************************************************************
 * MODULE VARIABLES
 **************************************************************************************************/
HWT906_LIB HWT906_Lib;
volatile union floatUnion AveragedIMUdata[6]; /* Buffer for IMU data to transmit*/
volatile uint32_t IMU_FramesCounter = 0;      /* tracks IMU frames collected. used to set IMU_Comm_Errors*/
uint8_t rxBuffer[PACKET_LEN];
uint32_t IMU_Comm_Errors = 0;


/***************************************************************************************************
 * PRIVATE FUNCTION PROTOTYPES
 **************************************************************************************************/
void AverageIMUdata(void); /* Function prototype for filtering IMU data */
float ConverToPolarCoordinates(float neg_pos_degrees);
void Service_HWT906(void);
/***************************************************************************************************
 * FUNCTION DEFINITIONS
 **************************************************************************************************/

extern "C" void USART0_Handler(void) 
{
  // 1) Grab & decode the status
  uint32_t sr = USART0->US_CSR;

  // 2) RXRDY?  Read RHR to clear it, then process the byte
  if (sr & US_CSR_RXRDY) 
  {
    uint8_t c = (uint8_t)USART0->US_RHR;
    // … push 'c' into your ring buffer or flag it . Process the full 33-byte buffer
    Service_HWT906();

    // 4) Configure PDC DMA for first 33-byte transfer
    Pdc* pdc = PDC_USART0;
    pdc->PERIPH_PTCR = PERIPH_PTCR_RXTDIS;
    pdc->PERIPH_RPR  = reinterpret_cast<uint32_t>(rxBuffer);
    pdc->PERIPH_RCR  = PACKET_LEN;

    // 6) Kick off RX DMA
    pdc->PERIPH_PTCR = PERIPH_PTCR_RXTEN;

  }

  // 3) OVRE?  Overrun happened—read RHR to clear the flag
  if (sr & US_CSR_OVRE) 
  {
    (void)USART0->US_RHR;
    // … optionally count overruns …
  }

  // 4) FRAME?  Framing error—read RHR to clear
  if (sr & US_CSR_FRAME) 
  {
    (void)USART0->US_RHR;
  }

  // // 5) PARE?  Parity error—read RHR to clear
   if (sr & US_CSR_PARE) 
   {
     (void)USART0->US_RHR;
   }

// Finally, clear the NVIC’s pending flag so the debugger
// (and CPU) won’t immediately re-enter this IRQ on resume
NVIC_ClearPendingIRQ(USART0_IRQn);

}
/***********************************************************************************************//**
 * @details     Initialize IMU pins and configuration. Enunciate when init
 **************************************************************************************************/
uint8_t HWT906_App :: Init(void)
{
  uint8_t initState = 0;

  // Route TX1/RX1 to USART0 peripheral A
// manually map D18/D19 to USART0 A-function
  PIO_Configure(
    g_APinDescription[PIN_RX1].pPort,
    PIO_PERIPH_A,
    g_APinDescription[PIN_RX1].ulPin,
    PIO_DEFAULT
  );
  PIO_Configure(
    g_APinDescription[PIN_TX1].pPort,
    PIO_PERIPH_A,
    g_APinDescription[PIN_TX1].ulPin,
    PIO_DEFAULT
  );


  // 2) Start Serial1 @115200
  Serial1.begin(230400);

  // 4) Configure PDC DMA for first 33-byte transfer
  Pdc* pdc = PDC_USART0;
  pdc->PERIPH_PTCR = PERIPH_PTCR_RXTDIS;
  pdc->PERIPH_RPR  = reinterpret_cast<uint32_t>(rxBuffer);
  pdc->PERIPH_RCR  = PACKET_LEN;

  // 5) Enable End-of-Reception interrupt
// 4. Disable _all_ USART0 interrupts, then enable only RXRDY
USART0->US_IDR = 0xFFFFFFFF;      // mask everything off
USART0->US_IER = US_IER_RXRDY     // data ready
               | US_IER_OVRE       // catch overruns
               | US_IER_FRAME      // framing errors
               | US_IER_PARE       // parity errors
               | US_IER_TIMEOUT;   // time-out indicator
  NVIC_EnableIRQ(USART0_IRQn);

  // 6) Kick off RX DMA
  pdc->PERIPH_PTCR = PERIPH_PTCR_RXTEN;
    
  HWT906_Lib.init();

  return initState;
}

/***********************************************************************************************//**
 * @details     Service IMU. Collect roll, pitch, yaw, and acceleration for all axes.
 **************************************************************************************************/
inline void Service_HWT906(void)
{
  /* Check if new IMU data is available */
  // When 33 bytes arrive, the IRQ sets dataReady
    for (int i = 0; i < 3; ++i)
        HWT906_Lib.parseFrame(&rxBuffer[i * 11]); // Parse each frame

    if (HWT906_Lib.haveFullTriplet()) 
	{
        auto acc   = HWT906_Lib.HWTgetAccel();
        //auto gyro  = HWT906_Lib.HWTgetGyro();
        auto euler = HWT906_Lib.HWTgetEuler();
        AveragedIMUdata[0].f = euler.x;
        AveragedIMUdata[1].f = euler.y;
        AveragedIMUdata[2].f = euler.z;
        AveragedIMUdata[3].f = acc.x;
        AveragedIMUdata[4].f = acc.y;
        AveragedIMUdata[5].f = acc.z;
        IMU_FramesCounter++;
    }
    else
    {
        IMU_Comm_Errors++;
    }
}

 /***********************************************************************************************//**
  * @details     periodic compare IMU_FramesCounter to verify new IMU data being collected
  **************************************************************************************************/
void HWT906_App :: CheckIMUDataCollection(void)
{
    static uint32_t lastIMU_FramesCounter = 0;

    if (IMU_FramesCounter == lastIMU_FramesCounter)
    {
        IMU_Comm_Errors++;
    }
    lastIMU_FramesCounter = IMU_FramesCounter;
    
}
