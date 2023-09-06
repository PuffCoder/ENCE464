/******************************************************************************
 *
 * pwm_task.c
 *
 * Purpose: 
 * Provides interfaces for PWM signal generation for helicopter's altitude and yaw control.
 *
 * Author: Heng Yin(hyi32)
 * Last modified: 09/08/2023
 *
*******************************************************************************/

// INCLUDES 
// ----------------------------------------------------------------------------

#include "config.h"          // System-wide configurations
#include "priorities.h"      // Task priority definitions
#include "pwm_task.h"        // PWM functionalities
#include "FreeRTOS.h"        // Core FreeRTOS functionalities
#include "task.h"            // FreeRTOS task functionalities
#include "queue.h"           // FreeRTOS queue functionalities
#include "semphr.h"          // FreeRTOS semaphore functionalities
#include "height_task.h"     // Helicopter data acquisition functionalities

// CONSTANTS-------------------------------------------------------------------

/** @brief Stack size (in words) for the PWM task. */
#define PWMTASKSTACKSIZE        128         

// GLOBAL VARIABLES------------------------------------------------------------

/** @brief Semaphore for UART operations. */
extern xSemaphoreHandle g_pUARTSemaphore;         

extern QueueHandle_t Q_tailDuty;
extern QueueHandle_t Q_mainDuty;

/** @brief Queue for altitude measurements display. */

/** @brief Altitude value used in PWM adjustments. */
extern uint32_t EXT_VAL;                          

// LOCAL FUNCTION PROTOTYPES---------------------------------------------------

void setMainPWM(uint32_t ui32Freq, uint32_t ui32Duty);
void setTailPWM(uint32_t ui32Freq, uint32_t ui32Duty);
void initTailMotorPWM(void);
void initMainMotorPWM(void);
void sendYawPercentageToQueue(void);
void sendHeightPercentageToQueue(void);


// FUNCTIONS-------------------------------------------------------------------
/**
 * @brief FreeRTOS task to manage PWM based on altitude data (EXT_VAL).
 *
 * This task is responsible for adjusting the PWM duty cycle based on altitude data.
 * It enables PWM outputs, periodically sends altitude data (as a percentage) to a queue,
 * and updates the PWM settings.
 *
 * @param pvParameters Task parameters (not utilized in this function).
 */
static void
PWM_Task(void *pvParameters)
{
    // Set the initial frequency for PWM
    uint32_t ui32Freq = PWM_START_RATE_HZ;
    uint32_t recieved_message;

    // Activate the PWM output for both the main and tail motors
    PWMOutputState(PWM_MAIN_BASE, PWM_MAIN_OUTBIT, true);
    PWMOutputState(PWM1_BASE, PWM_OUT_5_BIT, true);

    // Infinite loop to continuously update the PWM settings
    while(1)
    {

        if (xQueueReceive(Q_mainDuty, &recieved_message, 0) == pdPASS) {

            setMainPWM(ui32Freq, recieved_message);

        }

        if (xQueueReceive(Q_tailDuty, &recieved_message, 0) == pdPASS) {

            setTailPWM(ui32Freq, recieved_message);

        }

        vTaskDelay(100);
    }
}


/**
 * @brief Initialize the PWM settings for the main motor.
 */
void initMainMotorPWM(void)
{
    // Enable the PWM peripheral for the main motor
    SysCtlPeripheralEnable(PWM_MAIN_PERIPH_PWM);

    // Continuously poll until the PWM peripheral for the main motor is ready
    while(!SysCtlPeripheralReady(PWM_MAIN_PERIPH_PWM)) {
    }

    // Activate the GPIO peripheral associated with the PWM output for the main motor
    SysCtlPeripheralEnable(PWM_MAIN_PERIPH_GPIO);  

    // Set the GPIO pin to relay the PWM signal for the main motor
    GPIOPinConfigure(PWM_MAIN_GPIO_CONFIG);
    GPIOPinTypePWM(PWM_MAIN_GPIO_BASE, PWM_MAIN_GPIO_PIN);

    // Configure the PWM generator to operate in up-down counting mode without synchronization
    PWMGenConfigure(PWM_MAIN_BASE, PWM_MAIN_GEN,
                    PWM_GEN_MODE_UP_DOWN | PWM_GEN_MODE_NO_SYNC);

    // Initialize the PWM signal with a predefined frequency and duty cycle for the main motor
    // Constants `PWM_START_RATE_HZ` and `PWM_FIXED_DUTY` should be defined elsewhere
    setMainPWM(PWM_START_RATE_HZ, PWM_FIXED_DUTY);

    // Start the configured PWM generator for the main motor
    PWMGenEnable(PWM_MAIN_BASE, PWM_MAIN_GEN);

    // By default, turn off the PWM output for the main motor. It can be enabled later as needed.
    PWMOutputState(PWM_MAIN_BASE, PWM_MAIN_OUTBIT, false); 
}


/**
 * @brief Initialize the PWM settings for the tail motor.
 */
void initTailMotorPWM(void)
{
    // Enable the PWM peripheral for the tail motor
    SysCtlPeripheralEnable(PWM_TAIL_PERIPH_PWM);

    // Continuously poll until the PWM peripheral for the tail motor is ready
    // Print an error if initialization fails
    while(!SysCtlPeripheralReady(PWM_TAIL_PERIPH_PWM)) {
        //UARTprintf("Error: The PWM1 initialization has failed\n");
    }

    // Activate the GPIO peripheral associated with the PWM output for the tail motor
    SysCtlPeripheralEnable(PWM_TAIL_PERIPH_GPIO);  

    // Set the GPIO pin to relay the PWM signal
    GPIOPinConfigure(PWM_TAIL_GPIO_CONFIG);
    GPIOPinTypePWM(PWM_TAIL_GPIO_BASE, PWM_TAIL_GPIO_PIN);

    // Configure the PWM generator to operate in up-down counting mode without synchronization
    PWMGenConfigure(PWM_TAIL_BASE, PWM_TAIL_GEN,
                    PWM_GEN_MODE_UP_DOWN | PWM_GEN_MODE_NO_SYNC);

    // Initialize the PWM signal with a predefined frequency and duty cycle
    // Constants `PWM_START_RATE_HZ` and `PWM_FIXED_DUTY` should be defined elsewhere
    setTailPWM(PWM_START_RATE_HZ, PWM_FIXED_DUTY);

    // Start the configured PWM generator
    PWMGenEnable(PWM_TAIL_BASE, PWM_TAIL_GEN);

    // By default, turn off the PWM output. It can be enabled later as needed.
    PWMOutputState(PWM_TAIL_BASE, PWM_TAIL_OUTBIT, false); 
}


/**
 * @brief Initialize the PWM task and the associated hardware peripherals.
 *
 * This function initializes PWM for both main and tail motors. It also sets up 
 * and creates a FreeRTOS task for managing the PWM.
 *
 * @return uint32_t Returns 0 if initialization was successful, and 1 if there was an error.
 */
uint32_t PWMTaskInit(void)
{
    // Initialize the PWM for the main motor
    initMainMotorPWM();

    // Create the PWM task in FreeRTOS. The task will be named "pwmtask" and will run
    // at a priority level determined by `tskIDLE_PRIORITY + PRIORITY_PWM_TASK`.
    // If the task creation is not successful, return an error code.
    if (xTaskCreate(PWM_Task, (const portCHAR *)"pwmtask", PWMTASKSTACKSIZE, NULL,
                    tskIDLE_PRIORITY + PRIORITY_PWM_TASK, NULL) != pdTRUE) {
        return(1);  // Return 1 to indicate task creation error
    }

    return(0);  // Return 0 to indicate successful initialization
}



/**
 * @brief Set the frequency and duty cycle for the main PWM signal.
 *
 * This function adjusts both the frequency and the duty cycle of a PWM signal.
 * The frequency is determined by the divider and the system clock. The duty cycle 
 * determines the percentage of time the signal is in the 'high' state during each period.
 *
 * @param ui32Freq The desired frequency for the PWM signal.
 * @param ui32Duty The desired duty cycle for the PWM signal. This value represents 
 *                 the percentage of the time period during which the signal remains high.
 */
void setMainPWM(uint32_t ui32Freq, uint32_t ui32Duty)
{
    // Calculate the PWM period based on the desired frequency.
    // The period is inversely proportional to the frequency.
    // Formula: Period = System Clock frequency / (PWM divider * Target Frequency)
    uint32_t ui32Period = SysCtlClockGet() / PWM_DIVIDER / ui32Freq;

    // Set the calculated period for the PWM signal.
    PWMGenPeriodSet(PWM_MAIN_BASE, PWM_MAIN_GEN, ui32Period);

    // Calculate and set the duty cycle of the PWM signal.
    // The duty cycle determines the 'on' time for the signal during each period.
    // Formula: Pulse width (in clock cycles) = Period * (Duty Cycle Percentage / 100)
    PWMPulseWidthSet(PWM_MAIN_BASE, PWM_MAIN_OUTNUM, ui32Period * ui32Duty / 100);
}

/**
 * @brief Sets the frequency and duty cycle for the tail PWM signal.
 * 
 * This function calculates the appropriate PWM period based on the desired frequency
 * and then adjusts the pulse width according to the specified duty cycle.
 *
 * @param ui32Freq The desired frequency of the PWM signal in Hertz (Hz).
 * @param ui32Duty The desired duty cycle of the PWM signal in percentage (0 to 100).
 */
void setTailPWM(uint32_t ui32Freq, uint32_t ui32Duty)
{
    // Calculate the PWM period corresponding to the desired frequency.
    // The formula used: Period = Clock frequency / (PWM divider * Desired PWM frequency)
    uint32_t ui32Period = SysCtlClockGet() / PWM_DIVIDER / ui32Freq;

    // Set the period for the PWM signal
    PWMGenPeriodSet(PWM_TAIL_BASE, PWM_TAIL_GEN, ui32Period);

    // Set the pulse width (duty cycle) for the PWM signal.
    // The formula used: Pulse width = Period * Desired duty cycle / 100
    PWMPulseWidthSet(PWM_TAIL_BASE, PWM_TAIL_OUTNUM, ui32Period * ui32Duty / 100);
}





