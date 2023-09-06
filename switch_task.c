//*****************************************************************************
//
// switch_task.c - FreeRTOS task to process the button presses and send the
// relevant information using queues to other tasks to react.
//
// By Jamie Thomas - Group 9
// 12/08/2023
//
//*****************************************************************************

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
#include "switch_task.h"
#include "all_buttons.h"

//CONSTANTS--------------------------------------------------------------------
// The stack size for the switch task.
#define SWITCHTASKSTACKSIZE        128         // Stack size in words

// Variables for max values for height and yaw
#define HEIGHTMAXRANGE 10
#define YAWMAXRANGE 23

// Switch task delay variable
#define SWITCHTASKDELAY 25

//STATICS AND GLOBALS----------------------------------------------------------
// Queues and semaphores, externally defined.
extern xSemaphoreHandle g_pUARTSemaphore;
extern QueueHandle_t g_TargHeightDisplayQueue;
extern QueueHandle_t g_TargYawDisplayQueue;
extern QueueHandle_t g_TargYawControlQueue;
extern QueueHandle_t g_TargHeightControlQueue;

//LOCAL FUNCTION PROTOTYPES ---------------------------------------------------
bool checkButton(bool* prevButtonState, uint8_t buttonNumber);

//FUNCTIONS--------------------------------------------------------------------
//*****************************************************************************
//
// Reads the buttons' state and returns 1 if it has been pressed, and 0
// otherwise.
//
//*****************************************************************************
bool checkButton(
        bool* prevButtonState,
        uint8_t buttonNumber)
{
    bool curButtonState = ButtonsPoll(buttonNumber);

    if (curButtonState != *prevButtonState && curButtonState != 0) {
        *prevButtonState = curButtonState;
        return true;
    }
    else
    {
        *prevButtonState = curButtonState;
        return false;
    }
}

//*****************************************************************************
//
// This task reads the buttons' state and passes this information to the
// control task and display task through the relevant queues.
//
//*****************************************************************************
static void
SwitchTask(void *pvParameters)
{
    // Used for task delay timing
    portTickType ui16LastTime;
    uint32_t ui32SwitchDelay = SWITCHTASKDELAY;

    // True when a button is pressed which triggers the queues to be sent.
    bool buttonPressed = false;

    // Storing previous buttons' states
    bool ui8PrevButtonStateUp;
    ui8PrevButtonStateUp = false;

    bool ui8PrevButtonStateRight;
    ui8PrevButtonStateRight = false;

    bool ui8PrevButtonStateDown;
    ui8PrevButtonStateDown = false;

    bool ui8PrevButtonStateLeft;
    ui8PrevButtonStateLeft = false;


    // Initialise values for target height and yaw
    uint32_t ui32Height = 0;
    uint32_t ui32Yaw = 0;

    // Get the current tick count.
    ui16LastTime = xTaskGetTickCount();

    // Loop forever.
    while(1)
    {

        // Up Button.
        // Check if previous debounced state is equal to the current state.
        if(checkButton(&ui8PrevButtonStateUp, UP_BUTTON))
        {

            buttonPressed = true; // Trigger Queues

            // Adjust height value
            ui32Height += 1;
            if (ui32Height > HEIGHTMAXRANGE) {
                ui32Height = HEIGHTMAXRANGE;
            }

            // Guard UART from concurrent access.
            xSemaphoreTake(g_pUARTSemaphore, portMAX_DELAY);
            UARTprintf("Up Button is pressed.     \n");
            xSemaphoreGive(g_pUARTSemaphore);

        }

        // Right Button.
        // Check if previous debounced state is equal to the current state.
        if(checkButton(&ui8PrevButtonStateRight, RIGHT_BUTTON))
        {

            buttonPressed = true; // Trigger Queues

            // Adjust height value
            ui32Yaw += 1;
            if (ui32Yaw > YAWMAXRANGE) {
                ui32Yaw = 0;
            }

            // Guard UART from concurrent access.
            xSemaphoreTake(g_pUARTSemaphore, portMAX_DELAY);
            UARTprintf("Right Button is pressed.     \n");
            xSemaphoreGive(g_pUARTSemaphore);

        }

        // Down Button.
        // Check if previous debounced state is equal to the current state.
        if(checkButton(&ui8PrevButtonStateDown, DOWN_BUTTON))
        {

            buttonPressed = true; // Trigger Queues

            // Adjust height value
            if (ui32Height > 0) {
                ui32Height -= 1;
            }

            // Guard UART from concurrent access.
            xSemaphoreTake(g_pUARTSemaphore, portMAX_DELAY);
            UARTprintf("Down Button is pressed.     \n");
            xSemaphoreGive(g_pUARTSemaphore);

        }

        // Left Button.
        // Check if previous debounced state is equal to the current state.
        if(checkButton(&ui8PrevButtonStateLeft, LEFT_BUTTON))
        {

            buttonPressed = true; // Trigger Queue

            // Adjust yaw value
            if (ui32Yaw > 0) {
                ui32Yaw -= 1;
            } else {
                ui32Yaw = YAWMAXRANGE;
            }

            // Guard UART from concurrent access.
            xSemaphoreTake(g_pUARTSemaphore, portMAX_DELAY);
            UARTprintf("Left Button is pressed.     \n");
            xSemaphoreGive(g_pUARTSemaphore);

        }


        if (buttonPressed) {
            buttonPressed = false;

            // Pass the value of the new height to the display task.
            if(xQueueSend(g_TargHeightDisplayQueue, &ui32Height, portMAX_DELAY) !=
               pdPASS)
            {
                // Error. The queue should never be full. If so print the
                // error message on UART and wait for ever.
                //UARTprintf("\nQueue full. This should never happen.\n");
                while(1)
                {
                }
            }

            // Pass the value of the new yaw to the display task.
            if(xQueueSend(g_TargYawDisplayQueue, &ui32Yaw, portMAX_DELAY) !=
               pdPASS)
            {
                // Error. The queue should never be full. If so print the
                // error message on UART and wait for ever.
                //UARTprintf("\nQueue full. This should never happen.\n");
                while(1)
                {
                }
            }

            // Pass the value of the new height to the display task.
            if(xQueueSend(g_TargHeightControlQueue, &ui32Height, portMAX_DELAY) !=
               pdPASS)
            {
                // Error. The queue should never be full. If so print the
                // error message on UART and wait for ever.
                //UARTprintf("\nQueue full. This should never happen.\n");
                while(1)
                {
                }
            }

            // Pass the value of the new yaw to the display task.
            if(xQueueSend(g_TargYawControlQueue, &ui32Yaw, portMAX_DELAY) !=
               pdPASS)
            {
                // Error. The queue should never be full. If so print the
                // error message on UART and wait for ever.
                //UARTprintf("\nQueue full. This should never happen.\n");
                while(1)
                {
                }
            }

        }


        // Wait for the required amount of time to check back.
        vTaskDelayUntil(&ui16LastTime, ui32SwitchDelay / portTICK_RATE_MS);
    }
}

//*****************************************************************************
//
// Initializes the switch task.
//
//*****************************************************************************
uint32_t
SwitchTaskInit(void)
{

    // Unlock the GPIO LOCK register for Right button to work.
    HWREG(GPIO_PORTF_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY;
    HWREG(GPIO_PORTF_BASE + GPIO_O_CR) = 0xFF;

    // Initialize the buttons
    ButtonsInit();

    // Create the switch task.
    if(xTaskCreate(SwitchTask, (const portCHAR *)"Switch",
                   SWITCHTASKSTACKSIZE, NULL, tskIDLE_PRIORITY +
                   PRIORITY_SWITCH_TASK, NULL) != pdTRUE)
    {
        return(1);
    }

    // Success.
    return(0);
}
