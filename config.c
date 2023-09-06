/******************************************************************************i*
 *
 * G9_proConfig.c
 *
 * Configuration for the initialization
 *        [ UART ] 
 *
 * Heng Yin
 * Last modified:  04/08/2023
 *
*******************************************************************************/
#include "config.h"


//*****************************************************************************
//
// Configure the UART and its pins.  This must be called before UARTprintf().
//
//*****************************************************************************
extern void ConfigureUART(void)
{
    //
    // Enable the GPIO Peripheral used by the UART.
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    //
    // Enable UART0
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    //
    // Configure GPIO Pins for UART mode.
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    //
    // Use the internal 16MHz oscillator as the UART clock source.
    UARTClockSourceSet(UART0_BASE, UART_CLOCK_PIOSC);

    //
    // Initialize the UART for console I/O.
    UARTStdioConfig(0, 115200, 16000000);
}



void resetHardwareConfig(void)
{
    // 
    // reset the ADC0 peripheral 
    SysCtlPeripheralReset(SYSCTL_PERIPH_ADC0);  

    // 
    // reset the PWM0 peripheral 
    SysCtlPeripheralReset (PWM_MAIN_PERIPH_GPIO); 
    SysCtlPeripheralReset (PWM_MAIN_PERIPH_PWM); 
    SysCtlPeripheralReset (PWM_TAIL_PERIPH_PWM); 
    SysCtlPeripheralReset (PWM_TAIL_PERIPH_GPIO); 
}

