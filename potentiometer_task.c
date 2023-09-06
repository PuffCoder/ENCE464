/*
 * potentiometer_task.c
 *
 * Reads the potentiometer using potentiometer.c and displays the value
 * on UART.
 *
 *  Created on: 2/08/2023
 *      Author: Jamie Thomas
 */

//INCLUDES ----------------------------------------------------
#include <stdbool.h>
#include <stdint.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/rom.h"
#include "utils/uartstdio.h"
#include "priorities.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "potentiometer_task.h"
#include "potentiometer.h"

//CONSTANTS--------------------------------------------------------------------
// The stack size for the display task.
#define POTENTIOTASKSTACKSIZE        128         // Stack size in words

// Variance of the ADC value to help display the value smoothly
#define VARIANCE 5

//STATICS AND GLOBALS----------------------------------------------------------
// UART Semaphore for protecting UART cardinality.
extern xSemaphoreHandle g_pUARTSemaphore;

//FUNCTIONS--------------------------------------------------------------------
//*****************************************************************************
//
// This task reads the potentiometer's state and displays its change (if it's
// changed) on UART COM.
//
//*****************************************************************************
static void
PotentiometerTask(void *pvParameters)
{
    // Used for task delay timing
    portTickType ui16LastTime;
    uint32_t ui32SwitchDelay = 25;

    // Initialise states of buttons
    uint32_t ui32CurPotentioState, ui32PrevPotentioState;
    ui32CurPotentioState = ui32PrevPotentioState = 0;

    // Initialise variable for if the polling of the potentiometer
    // gave back a new value.
    bool pollSuccess = false;

    // Get the current tick count.
    ui16LastTime = xTaskGetTickCount();

    while(1)
    {
        // Poll the state of the potentiometer.
        pollSuccess = PotentiometerPoll(&ui32CurPotentioState);

        // Check that the function returned something. (Actually polled the ADC)
        if (pollSuccess)
        {
            // Check if previous state is equal to the current state.
            if( !(ui32PrevPotentioState <= ui32CurPotentioState + VARIANCE &&
                    ui32CurPotentioState <= ui32PrevPotentioState + VARIANCE))
            {
                ui32PrevPotentioState = ui32CurPotentioState;

                // Display that potentiometer has changed.

                // Guard UART from concurrent access.
                xSemaphoreTake(g_pUARTSemaphore, portMAX_DELAY);
                UARTprintf(
                        "Potentiometer changed, Value: '%d'.\n",
                        ui32CurPotentioState
                );
                xSemaphoreGive(g_pUARTSemaphore);
            }
        }

        pollSuccess = false;

        // Wait for the required amount of time to check back.
        vTaskDelayUntil(&ui16LastTime, ui32SwitchDelay / portTICK_RATE_MS);
    }

}

//*****************************************************************************
//
// Initializes the potentiometer task.
//
//*****************************************************************************
uint32_t
PotentiometerTaskInit(void)
{

    // Initialize the potentiometer
    PotentiometerInit();

    // Create the potentiometer task.
    if(xTaskCreate(PotentiometerTask, (const portCHAR *)"Potentiometer",
                   POTENTIOTASKSTACKSIZE, NULL, tskIDLE_PRIORITY +
                   PRIORITY_POTENTIOMETER_TASK, NULL) != pdTRUE)
    {
        return(1);
    }

    // Success.
    return(0);
}


