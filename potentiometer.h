/*
 * potentiometer.h
 *
 *  Created on: 2/08/2023
 *      Author: Jamie Thomas
 */

#ifndef __POTENTIOMETER_H__
#define __POTENTIOMETER_H__

//*****************************************************************************
//
// The potentiometer is connected to ADC0.
//
//*****************************************************************************

// Public Functions
void PotentiometerInit(void);
bool PotentiometerPoll(uint32_t* ui32Out);

#endif /* __POTENTIOMETER_H__ */
