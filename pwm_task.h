/******************************************************************************
 *
 * pwm_task.h
 *
 * Purpose: 
 * Provides interfaces for PWM signal generation for helicopter's altitude and yaw control.
 *
 * Author: Heng Yin(hyi32)
 * Last modified: 09/08/2023
 *
*******************************************************************************/
#ifndef __PWM_TASK_H__
#define __PWM_TASK_H__

#include <stdint.h>

uint32_t PWMTaskInit(void);

void initTailMotorPWM(void);

#endif /* __PWM_TASK_H__ */
