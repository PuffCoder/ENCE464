/*
 * potentiometer.c
 *
 * Sets up and polls the potentiometer on the Tiva Board.
 *
 *  Created on: 2/08/2023
 *      Author: Jamie Thomas
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "inc/hw_gpio.h"
#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/adc.h"
#include "circBufT.h"
#include "potentiometer.h"

//CONSTANTS--------------------------------------------------------------------
#define BUF_SIZE 10  // Size of circular buffer
#define SAMPLE_SEQUENCE 3  // Which sequence to read ADC0 using

//STATICS AND GLOBALS----------------------------------------------------------
static circBuf_t g_inBuffer; // Buffer of size BUF_SIZE integers (sample values)

//LOCAL FUNCTION PROTOTYPES ---------------------------------------------------
static uint32_t readAverageCircBuf(circBuf_t *buffer);

//FUNCTIONS--------------------------------------------------------------------
//*****************************************************************************
//
// Polls the potentiometer and returns a boolean as to whether the ADC
// was polled. Returns the ADC value between 0 and 4095 through ui32Out.
//
//*****************************************************************************
bool
PotentiometerPoll(uint32_t* ui32Out)
{

    uint32_t ui32ADCData;
    uint8_t firstTime = 1;


    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_ADC0))
    {
    }
    if (SysCtlPeripheralReady(SYSCTL_PERIPH_ADC0))
    {
        ADCSequenceConfigure(ADC0_BASE, SAMPLE_SEQUENCE, ADC_TRIGGER_PROCESSOR, 0);
        ADCSequenceStepConfigure(ADC0_BASE, SAMPLE_SEQUENCE, 0,
        ADC_CTL_IE | ADC_CTL_END | ADC_CTL_CH0);
        ADCSequenceEnable(ADC0_BASE, SAMPLE_SEQUENCE);

        // Trigger the sample sequence.
        ADCProcessorTrigger(ADC0_BASE, SAMPLE_SEQUENCE);

        // Wait until the sample sequence has completed.
        while(!ADCIntStatus(ADC0_BASE, SAMPLE_SEQUENCE, false))
        {
        }

        // Read the value from the ADC.
        ADCSequenceDataGet(ADC0_BASE, SAMPLE_SEQUENCE, &ui32ADCData);

        // To make sure the buffer is initialised with
        // valid values so it doesn't ramp up from 0.
        if (firstTime) {
            firstTime = 0;
            uint8_t i;
            for (i = 0; i < BUF_SIZE + BUF_SIZE; ++i) {
                writeCircBuf(&g_inBuffer, ui32ADCData);
            }
        }

        writeCircBuf(&g_inBuffer, ui32ADCData);

        *ui32Out = readAverageCircBuf(&g_inBuffer);

        return 1;
    }

    return 0;

}

//*****************************************************************************
//
//! Initializes the components used by the board's potentiometer.
//!
//! This function must be called during application initialization to
//! configure the circular buffer to which the potentiometer writes to.
//
//*****************************************************************************
void
PotentiometerInit(void)
{

    if (initCircBuf(&g_inBuffer, BUF_SIZE) == NULL)
    {
        // Failed to initialize the buffer. Check heap size is set to
        // heap_size > 0 (say, 128) in CCS.
        while(1) {}
    }

    SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);


}


//*****************************************************************************
//
// Function to calculate the mean value of samples within a circular buffer
//
// Function used from ADCdemo1.c
//
//*****************************************************************************
uint32_t readAverageCircBuf(circBuf_t *buffer)
{
    // Initialize variables
    uint32_t sum = 0;
    uint8_t i;

    // Sum buffer values
    for (i = 0; i < BUF_SIZE; i++) {
        sum = sum + readCircBuf(buffer);
    }

    // return the average of the values
    return (2 * sum + BUF_SIZE) / 2 / BUF_SIZE;
}


