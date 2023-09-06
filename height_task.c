/*******************************************************************************
 *
 * height_task.c
 *
 * This module deals with the task for updating the ADC value 
 * and storing this value into a circular buffer.
 *
 * Authors: Heng Yin (hyi32) & Franco (wly13)
 * Last modified: 04/08/2023
 *
*******************************************************************************/

// INCLUDES -----------------------------------------------------------------------

#include <stdint.h>               // Standard integer types
#include "config.h"               // Configuration parameters for the system
#include "circBufT.h"             // Circular Buffer Type Definitions and functions

// FreeRTOS includes
#include "priorities.h"           // Task priorities definitions
#include "FreeRTOS.h"             // Core FreeRTOS includes
#include "task.h"                 // FreeRTOS task utilities
#include "semphr.h"               // FreeRTOS semaphore utilities
#include "yaw_task.h"

// CONSTANTS ----------------------------------------------------------------------
// The stack size is defined for the rig task, setting its memory allocation.
#define RIGTASKSTACKSIZE        128         // Stack size (in words) allocated for the rigTask

// STATICS AND GLOBAL VARIABLES ---------------------------------------------------
// altitude_Buf is a circular buffer that holds altitude data from the ADC readings.
static circBuf_t altitude_Buf;              // Circular buffer for storing altitude data

// g_pUARTSemaphore is a semaphore used to synchronize UART operations. 
// It's declared externally, probably in a header file or another source file.
extern xSemaphoreHandle g_pUARTSemaphore;   // Semaphore handle for UART operations (declared in another file)
extern xSemaphoreHandle g_ControlSemaphore;
extern xSemaphoreHandle g_ADCSemaphore;

extern QueueHandle_t g_MeasHeightDisplayQueue;
extern QueueHandle_t g_MeasHeightControlQueue;    
extern QueueHandle_t g_MeasYawControlQueue;

// EXT_VAL holds a computed mean altitude value which can be used elsewhere in the program.
uint32_t EXT_VAL;                           // Global variable to store the mean altitude value


// LOCAL FUNCTION PROTOTYPES -----------------------------------------------------

/**
 * Function to calculate the mean altitude from ADC readings stored in the buffer.
 *
 * @param adder Pointer to the circular buffer holding ADC values.
 * @return Mean altitude value calculated from the ADC readings.
 */
uint32_t meanAltiduteADC(circBuf_t *adder);

/**
 * Task function to continuously read altitude values from ADC 
 * and store them in the circular buffer.
 * 
 * @param pvParameters Pointer to task-specific data (unused in this context).
 */
static void rigTask(void *pvParameters);

void sendMeasHeightToBOTH(void);



//FUNCTIONS ---------------------------------------------------------------------
/**
 * Task function to continuously read altitude values from the ADC 
 * and store them in a circular buffer. This task also calculates the 
 * average altitude value and makes it available via the EXT_VAL global variable.
 * 
 * @param pvParameters Pointer to task-specific data (unused in this context).
 */
static void rigTask(void *pvParameters)
{
    uint32_t altitude_Val;  // Variable to store the current altitude value

    // Infinite loop to keep the task running
    while(1){
        
        int test = uxSemaphoreGetCount(g_ControlSemaphore);
        int test1 = uxSemaphoreGetCount(g_ADCSemaphore);
        if ((xSemaphoreTake(g_ADCSemaphore, 1)) == pdTRUE) {

            // Trigger the ADC to start a conversion and get the altitude value
            ADCProcessorTrigger(ADC0_BASE, 3);

            // Retrieve the converted value from ADC and store in altitude_Val
            ADCSequenceDataGet(ADC0_BASE, 3, &altitude_Val);

            // Write the retrieved value to the circular buffer
            writeCircBuf(&altitude_Buf, altitude_Val);

            // Calculate the average altitude value using values in the circular buffer
            // and store the result in the global variable EXT_VAL
            EXT_VAL = meanAltiduteADC(&altitude_Buf);

            int32_t current_yaw = get_current_yaw();

            if(xQueueSend(g_MeasYawControlQueue, &current_yaw, 10) != pdPASS)

            {
                // This section is triggered if the queue overflows.
                while(1)  // Never-ending loop
                {
                }
            }

            sendMeasHeightToBOTH();

            xSemaphoreGive(g_ControlSemaphore);

            vTaskDelay(1);

        }

    }
}


/**
 * Initializes the rig data module, which involves setting up the circular buffer 
 * and creating the associated task.
 * 
 * @return uint32_t Returns 0 if initialization was successful, and 1 if there was an error.
 */
uint32_t HEIGHTTaskInit(void)
{
    // Initialize the circular buffer to store altitude data
    initCircBuf(&altitude_Buf, BUF_SIZE);

    // Create a FreeRTOS task for updating the altitude data
    if (xTaskCreate(rigTask,                 // Task function
                    (const portCHAR *)"RIG", // Task name for debugging purposes
                    RIGTASKSTACKSIZE,        // Stack size
                    NULL,                    // Task parameters
                    tskIDLE_PRIORITY + PRIORITY_HEIGHT_TASK, // Task priority
                    NULL) != pdTRUE) {      // Task handle
        return(1);  // Return 1 if task creation failed
    }

    // Return 0 indicating successful initialization
    return(0);
}


/**
 * Calculates the mean value of the altitude ADC readings from a circular buffer.
 * 
 * @param adder  Pointer to the circular buffer containing ADC values.
 * 
 * @return       The calculated mean value.
 */
uint32_t meanAltiduteADC(circBuf_t *adder)
{
    int32_t sum = 0;         // Initialize sum to store the total of all readings
    int i;                   // Index for the loop
    
    // Iterate over the buffer to sum up all the readings
    for (i = 0; i < adder->size; i++)
        sum = sum + readCircBuf(adder);

    // Compute the mean value. The formula seems to be averaging the sum in a specific way.
    sum = (2 * sum + adder->size) / 2 / adder->size;
    
    return sum;              // Return the calculated mean value
}


void initADC(void)
{
    // Enable ADC0 peripheral
    SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);
    
    /* / The ADC0 peripheral must be enabled for configuration and use. */
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
    GPIOPinTypeADC(GPIO_PORTE_BASE,GPIO_PIN_4);

    // Wait until the ADC0 peripheral is ready
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_ADC0)) {
        //UARTprintf("Error: The ADC0 initialization has failed\n");
    }

    // Configure ADC0 sample sequence 3 for processor-triggered conversions
    ADCSequenceConfigure(ADC0_BASE, 3, ADC_TRIGGER_PROCESSOR, 0);

    // Configure step 0 on sequence 3 for the potential meter channel.
    // This step is set as the last step of the sequence, with both the
    // interrupt flag (ADC_CTL_IE) and end flag (ADC_CTL_END) set.
    ADCSequenceStepConfigure(ADC0_BASE, 3, 0, altitudeChannel | ADC_CTL_IE | ADC_CTL_END);

    // Enable ADC0 sample sequence 3
    ADCSequenceEnable(ADC0_BASE, 3);

    // Uncomment the below line if you want to use an interrupt handler for ADC readings
    // ADCIntRegister (ADC0_BASE, 3, ADCIntHandler);

    // Enable interrupts for ADC0 sequence 3 and clear any outstanding interrupts
    ADCIntEnable(ADC0_BASE, 3);
}




/**
 * Convert the given ADC value to a percentage representation.
 * This can be used to adjust the PWM output according to the ADC value.
 *
 * @param value The ADC value to be converted.
 * @return The percentage representation of the ADC value.
 */
uint32_t 
convert_to_percentage(uint32_t value) 
{
    return (value * 1000 + ADC_MAX_VALUE / 2) / ADC_MAX_VALUE;
}


void sendMeasHeightToBOTH(void)
{
    static int count = 0;

    // Dispatch the transformed value to the message queue. In the event that the dispatch fails (owing to a full queue),
    // an error is triggered.
    if (count > 3) {

        count = 0;
        if(xQueueSend(g_MeasHeightDisplayQueue, &EXT_VAL, 10) != pdPASS)

        {
            // This section is triggered if the queue overflows.
            while(1)  // Never-ending loop
            {
            }
        }
    } else {

        count++;
    }

    if(xQueueSend(g_MeasHeightControlQueue, &EXT_VAL, 10) != pdPASS)

    {
        // This section is triggered if the queue overflows.
        while(1)  // Never-ending loop
        {
        }
    }


}




