/*******************************************************
 *
 * height_task.h
 *
 * Header file for rigData.c
 * Provides interfaces for getting altitude and yaw data from the HELI.
 * Supports initialization of the ADC and storing its values into a circular buffer.
 *
 * Heng Yin (hyi32) & Franco (wly13)
 * Last modified:  04/08/2023
 *
*******************************************************/

#ifndef __HEIGHT_TASK_H__
#define __HEIGHT_TASK_H__

// Include the header for circular buffer type definitions and functions.
#include "circBufT.h"

// Function prototypes.

// Initializes the rig data infrastructure, 
// returns a non-zero value in case of an error.
extern uint32_t HEIGHTTaskInit(void);

// Sets up the ADC peripheral for reading altitude values.
void initADC(void);

#endif /* height_task*/
