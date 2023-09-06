/**
 * @file yaw_task.c
 * @author [Heng Yin] -- hyi32
 * @date [17 Aug 2023]
 * @brief Implementation of the yaw task for [Project Name].
 **/


// INCLUDES -------------------------------------------------------------------
// Standard includes
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

// Project specific includes
#include "config.h"
#include "circBufT.h"
#include "priorities.h"

// FreeRTOS includes
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"

// Yaw task specific includes
#include "yaw_task.h"
#include "display_task.h"

// CONSTANTS ------------------------------------------------------------------
#define RIGTASKSTACKSIZE        128     // Stack size for the tasks (in words)

// Yaw task specific constants and configurations

#define FIRST_BIT 0b00000001    // Bit mask for the first bit
#define SECOND_BIT 0b00000010   // Bit mask for the second bit

// External semaphore for UART operations
extern xSemaphoreHandle g_pUARTSemaphore;
extern QueueHandle_t g_MeasYawControlQueue;
extern QueueHandle_t g_MeasYawDisplayQueue;

// Global variables for tracking GPIO states and yaw orientation
volatile bool prev_State_A = 0;       // Previous state for Phase A
volatile bool prev_State_B = 0;       // Previous state for Phase B
volatile bool current_State_A = 0;    // Current state for Phase A
volatile bool current_State_B = 0;    // Current state for Phase B
static int8_t orientation = 0;         // Current orientation based on GPIO states

// Yaw config
const uint32_t yaw_ticks = 448;
const float yaw_angle_ratio = 360.0 / 448.0;
static int32_t yaw_counter = 0;
static int32_t yaw_degree = 0;

// LOCAL FUNCTION PROTOTYPES ---------------------------------------------------


// Reads the GPIO inputs and updates the yaw direction.
void updateDirection(void);

// Calculates the yaw angle based on the direction.
void counterYaw(void);

// Converts the yaw counter value to degrees.
void convert_to_degree(void);

// ISR for when a reference pin is found.
void ISR_FOUND_REF(void);

// FUNCTIONS---------------------------------------------------------------------
/**
 * @brief FreeRTOS Task for monitoring and handling the yaw orientation.
 *
 * This task continuously prints the yaw direction (or yaw status) to UART. 
 * A mutual exclusion semaphore (g_pUARTSemaphore) is used to ensure exclusive 
 * access to the UART resource, thus avoiding any potential contention issues.
 * After printing to UART, the task sleeps (delays) for 100 milliseconds before 
 * the next iteration.
 *
 * @note The UARTprintf() function is assumed to output text data via UART.
 *
 * @warning Ensure the g_pUARTSemaphore has been initialized before this task 
 * is started. Otherwise, it will lead to undefined behavior.
 *
 * @param pvParameters Parameters for the task (not used in this context).
 *
 */
static void yawTask(void *pvParameters)
{
    // Continuous loop for the task
    while(1)
    {
        convert_to_degree();
        
        // Release the UART semaphore after use
        xSemaphoreGive(g_pUARTSemaphore);
        

        // Pass the value of the new yaw to the display task.
        if(xQueueSend(g_MeasYawDisplayQueue, &yaw_degree, portMAX_DELAY) != pdPASS)
        {
        //
        // Error. The queue should never be full. If so print the
        // error message on UART and wait for ever.
          UARTprintf("\nQueue full. This should never happen.\n");
          while(1)
          {
          }
        }

        // Delay the task for 100 milliseconds before the next iteration
        vTaskDelay(100);
    }
}


/**
 * @brief Initializes the Yaw Task for execution.
 *
 * This function creates a new FreeRTOS task for yaw monitoring and handling.
 * The task is created using xTaskCreate(), which is part of the FreeRTOS API.
 *
 * @note The task is created with a specified stack size (RIGTASKSTACKSIZE) and 
 * priority level (PRIORITY_YAW_TASK above the idle task priority).
 *
 * @warning Ensure that the relevant parameters and constants like RIGTASKSTACKSIZE,
 * tskIDLE_PRIORITY, and PRIORITY_YAW_TASK are correctly defined elsewhere in 
 * the codebase.
 *
 * @return uint32_t - Returns '0' on successful task creation, '1' otherwise.
 */
uint32_t YAWTaskInit(void)
{
    // Create the Yaw task using FreeRTOS's xTaskCreate API
    if (xTaskCreate(yawTask, (const portCHAR *)"Yaw", RIGTASKSTACKSIZE, NULL,
                    tskIDLE_PRIORITY + PRIORITY_YAW_TASK, NULL) != pdTRUE) {
        // Task creation failed
        return(1);
    }

    // Task creation was successful
    return(0);
}




// Interrupt Service Routine for direction detection
void ISR_GET_DIRECTION() 
{
    // Check the current direction based on the GPIO pins state
    updateDirection();  // Determine the direction of rotation based on GPIO inputs
    counterYaw();       // Increment or decrement yaw counter based on orientation

    // Clear the interrupt flags for the GPIO pins
    GPIOIntClear(GPIO_PORTB_BASE, PHASE_A | PHASE_B);
}


/**
 * @brief Initializes the yaw detection using GPIO pins.
 *
 * This function sets up the necessary configurations to detect yaw using the PHASE_A
 * and PHASE_B GPIO pins. It enables the required peripheral, waits until it's ready,
 * registers the interrupt handler, configures the pin settings, and enables the
 * required interrupt types for both rising and falling edges on the specified pins.
 */
void initialiseYaw(void) {
    // Create the semaphore

    // Enable the GPIOB peripheral
    SysCtlPeripheralEnable(PHASE_PERIPH);
    
    // Wait for the GPIOB module to be ready
    while (!SysCtlPeripheralReady(PHASE_PERIPH)) { }

    // Register the port-level interrupt handler
    // This handler is the primary interrupt handler for all pin interrupts
    GPIOIntRegister(PHASE_PORT, ISR_GET_DIRECTION);

    // Configure GPIO pin settings
    // Set pin 0 and 1 as input
    GPIOPinTypeGPIOInput(PHASE_PORT, PHASE_A | PHASE_B);

    // Set pin 0 and 1 as rising and falling edge triggered interrupt
    GPIOIntTypeSet(PHASE_PORT, PHASE_A | PHASE_B, GPIO_BOTH_EDGES);
    GPIOIntEnable(PHASE_PORT, PHASE_A | PHASE_B);
}


/**
 * @brief Determines yaw direction from GPIO inputs using a lookup table.
 *
 * Reads GPIO pins (PHASE_A and PHASE_B), maps to orientation (CW, CCW, still),
 * and updates previous state for subsequent calls.
 */
void updateDirection(void) {
    // Read the output of the GPIO pins
    current_State_A = (GPIOPinRead(PHASE_PORT, PHASE_A));
    current_State_B = ((GPIOPinRead(PHASE_PORT, PHASE_B)) >> 1);
    
    // Convert the results into a 4 bits binary number which will be used as the index of the lookup table
    int32_t index = (prev_State_B << 3) + (prev_State_A << 2) + (current_State_B << 1) + current_State_A;
    
    // Find direction in the lookup table
    orientation = Dir_List[index];
    
    // Update the previous GPIO outputs value to the current value
    prev_State_A = current_State_A;
    prev_State_B = current_State_B;
}

int32_t get_current_yaw(void)
{

    return yaw_degree;
}

/**
 * @brief This function calculates the yaw orientation based on the current direction.
 * The orientation could be either clockwise, anti-clockwise or still.
 */
void counterYaw(void) {
    switch (orientation) {
        case ANTICLOCKWISE:
            yaw_counter--;  // Uncomment this to decrease yaw value for anti-clockwise rotation
            break;
        case CLOCKWISE:
            yaw_counter++;  // Uncomment this to increase yaw value for clockwise rotation
            break;
        case STILL:
            // No action is taken if the orientation is still
            break;
    }
}


// Convert the yaw counter value to degrees
void convert_to_degree(void)
{
    // Check if yaw_counter is greater than 224
    if (yaw_counter > 224) {
        yaw_counter -= yaw_ticks; // Adjust yaw_counter by subtracting yaw_ticks
    }

    // Check if yaw_counter is less than -223
    else if (yaw_counter < -223) {
        yaw_counter += yaw_ticks; // Adjust yaw_counter by adding yaw_ticks
    }

    // Calculate the yaw degree using the yaw_counter and yaw_angle_ratio
    yaw_degree = yaw_counter * yaw_angle_ratio;
}




// Initialize the yaw reference pin 
void initYawRef(void) {
    // Enable the peripheral for the yaw reference
    SysCtlPeripheralEnable(YAW_REF_PERIPH);

    // Set the yaw reference pin as an input
    GPIOPinTypeGPIOInput(YAW_REF_PORT, YAW_REF_PIN);

    // Configure the pin with 4mA drive strength and pull-up resistor
    GPIOPadConfigSet(YAW_REF_PORT, YAW_REF_PIN, GPIO_STRENGTH_4MA, GPIO_PIN_TYPE_STD_WPU);

    // Configure the interrupt type for falling edge
    GPIOIntTypeSet(YAW_REF_PORT, YAW_REF_PIN, GPIO_FALLING_EDGE);

    // Register the ISR (Interrupt Service Routine) for the yaw reference pin
    GPIOIntRegister(YAW_REF_PORT, ISR_FOUND_REF);

    // Enable interrupts for the yaw reference pin
    GPIOIntEnable(YAW_REF_PORT, YAW_REF_PIN);
}


// TO-DO fond the orientation
void ISR_FOUND_REF(void)
{
    //UARTprintf("Found the pin,    The Pin is: %d\n");
    GPIOIntDisable(YAW_REF_PORT, YAW_REF_PIN);
    GPIOIntClear(YAW_REF_PORT, YAW_REF_PIN);

}


