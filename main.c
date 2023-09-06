#include <stdbool.h>
#include <stdint.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "driverlib/adc.h"
#include "utils/uartstdio.h"
// freeRTOS header files
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
// 
#include "config.h"
#include "height_task.h"
#include "pwm_task.h"
#include "potentiometer_task.h"
#include "display_task.h"
#include "switch_task.h"
#include "led_task.h"
#include "all_buttons.h"
#include "yaw_task.h"
#include "control_task.h"

//*****************************************************************************
//
// The mutex that protects concurrent access of UART from multiple tasks.
// Also semaphores used to protect the queues from overflowing between
// the control task (control_task) and the ADC task (height_task).
//
//*****************************************************************************
xSemaphoreHandle g_pUARTSemaphore;
xSemaphoreHandle g_ControlSemaphore;
xSemaphoreHandle g_ADCSemaphore;

QueueHandle_t Q_tailDuty;
QueueHandle_t Q_mainDuty;


//*****************************************************************************
//
// The error routine that is called if the driver library encounters an error.
//
//*****************************************************************************
#ifdef DEBUG
void
__error__(char *pcFilename, uint32_t ui32Line)
{
}

#endif

//*****************************************************************************
//
// This hook is called by FreeRTOS when an stack overflow error is detected.
//
//*****************************************************************************
void
vApplicationStackOverflowHook(xTaskHandle *pxTask, char *pcTaskName)
{
    //
    // This function can not return, so loop forever.  Interrupts are disabled
    // on entry to this function, so no processor interrupts will interrupt
    // this loop.
    //
    while(1)
    {
    }
}

//*****************************************************************************
//
// Initialize FreeRTOS and start the initial set of tasks.
//
//*****************************************************************************
int
main(void)
{
    // Resets hardware config to allow for reconfiguration
    resetHardwareConfig();

    // Set the clocking to run at 50 MHz from the PLL.
    SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_XTAL_16MHZ |
                       SYSCTL_OSC_MAIN);

    // Initialize the UART and configure it for 115,200, 8-N-1 operation.
    ConfigureUART();

    initADC();
    initialiseYaw();
    initYawRef();

    initTailMotorPWM();

    // Create a mutex to guard the UART.
    g_pUARTSemaphore = xSemaphoreCreateMutex();

    // Initialise queues for tail and main motor PWM signals
    Q_tailDuty = xQueueCreate(1,sizeof(uint32_t));
    Q_mainDuty = xQueueCreate(1,sizeof(uint32_t));

    g_ControlSemaphore = xSemaphoreCreateBinary();
    g_ADCSemaphore = xSemaphoreCreateBinary();
    xSemaphoreGive(g_ADCSemaphore);
    int test = uxSemaphoreGetCount(g_ControlSemaphore);
    

    // Create the button control task
     if(SwitchTaskInit() != 0)
     {

         while(1)
         {
         }
     }


    // Create the display task
    if (init_display() != 0){

        while(1){}
    }

    // Create the control task
    if (init_control() != 0){

        while(1){}
    }

    // Create the update altitude task
   if(HEIGHTTaskInit() != 0)
   {

       while(1)
       {
       }
   }


    // PWM output control the main and tail motor
    if(PWMTaskInit() != 0)
    {

        while(1)
        {
        }
    }
    

    // Create the update yaw task
    if(YAWTaskInit() != 0)
    {

        while(1)
        {
        }
    }


    // Start the scheduler.  This should not return.
    vTaskStartScheduler();

    // In case the scheduler returns for some reason, print an error and loop
    // forever.
    while(1)
    {
    }
}


// This is an error handling function called when FreeRTOS asserts.
// This should be used for debugging purposes
void vAssertCalled( const char * pcFile, unsigned long ulLine ) {
    (void)pcFile; // unused
    (void)ulLine; // unused
    while (1);
}
