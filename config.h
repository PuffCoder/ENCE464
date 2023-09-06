#ifndef __CONFIG_H__
#define __CONFIG_H__

/*******************************************************
 *
 * Config.h
 *
 * Configuration for the initialization
 *        [ UART ] 
 *
 * Heng Yin
 * Last modified: 04/08/2023
 *
*******************************************************/

// Standard C Libraries
#include <stdbool.h>
#include <stdint.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/adc.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/pwm.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/uart.h"
#include "utils/uartstdio.h"
#include "utils/ustdlib.h"


// CONSTANTS -------------------------------------------------------------------
//  ******************************* ADC ****************************************
// * ADC pin
#define potentialMeterPin GPIO_PIN_3
#define altitudePin GPIO_PIN_4 
// ADC channel
#define potentialMeterChannel ADC_CTL_CH0
#define altitudeChannel ADC_CTL_CH9 
// 12 bit ADC maximum value
#define ADC_MAX_VALUE 4095

//  ******************************* CircBuf ***********************************
#define BUF_SIZE 5      // Buffer size for Altitude

//  ******************************* PWM GPIO **********************************
//  ****** Main Motor 
#define PWM_MAIN_BASE PWM0_BASE
#define PWM_MAIN_GEN PWM_GEN_3
#define PWM_MAIN_OUTNUM PWM_OUT_7
#define PWM_MAIN_OUTBIT PWM_OUT_7_BIT
#define PWM_MAIN_PERIPH_PWM SYSCTL_PERIPH_PWM0
#define PWM_MAIN_PERIPH_GPIO SYSCTL_PERIPH_GPIOC
#define PWM_MAIN_GPIO_BASE GPIO_PORTC_BASE
#define PWM_MAIN_GPIO_CONFIG GPIO_PC5_M0PWM7
#define PWM_MAIN_GPIO_PIN GPIO_PIN_5

//  ****** Tail Motor 
#define PWM_TAIL_BASE PWM1_BASE
#define PWM_TAIL_GEN PWM_GEN_2
#define PWM_TAIL_OUTNUM PWM_OUT_5
#define PWM_TAIL_OUTBIT PWM_OUT_5_BIT
#define PWM_TAIL_PERIPH_PWM SYSCTL_PERIPH_PWM1
#define PWM_TAIL_PERIPH_GPIO SYSCTL_PERIPH_GPIOF
#define PWM_TAIL_GPIO_BASE GPIO_PORTF_BASE
#define PWM_TAIL_GPIO_CONFIG GPIO_PF1_M1PWM5
#define PWM_TAIL_GPIO_PIN GPIO_PIN_1

//  ******************************* PWM Setting *******************************
#define PWM_START_RATE_HZ  250
#define PWM_RATE_STEP_HZ   50
#define PWM_RATE_MIN_HZ    50
#define PWM_RATE_MAX_HZ    400
#define PWM_FIXED_DUTY     7 
#define PWM_DIVIDER_CODE   SYSCTL_PWMDIV_4
#define PWM_DIVIDER        4
//*****************************************************************************

//  ******************************* YAW Config *******************************
#define PHASE_PERIPH SYSCTL_PERIPH_GPIOB
#define PHASE_PORT GPIO_PORTB_BASE

#define PHASE_A GPIO_PIN_0      // GPIO Pin for Phase A
#define PHASE_B GPIO_PIN_1      // GPIO Pin for Phase B
#define YAW_REF_PERIPH SYSCTL_PERIPH_GPIOC
#define YAW_REF_PORT GPIO_PORTC_BASE
#define YAW_REF_PIN GPIO_PIN_4


//*****************************************************************************

// Function declarations for UART config
extern void ConfigureUART(void);
// void initADC(void);
void resetHardwareConfig(void);
/* void initialiseYaw(void); */


#endif // 
