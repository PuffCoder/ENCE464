/*
 * all_buttons.h
 *
 *  Created on: 5/08/2023
 *      Author: Jamie Thomas
 */

#ifndef __ALL_BUTTONS_H__
#define __ALL_BUTTONS_H__

#include <stdbool.h>
#include <stdint.h>

#define NUMOFBUTTONS 4

enum {
   UP_BUTTON = 0,
   RIGHT_BUTTON,
   DOWN_BUTTON,
   LEFT_BUTTON
} button_choice;

typedef struct buttons_t {
    uint32_t base;
    uint32_t periph;
    uint32_t pin;
    uint32_t pull_up;
    uint8_t button;
} button_t;

// Public Functions
void ButtonsInit(void);
bool ButtonsPoll(uint8_t button);




#endif /* __ALL_BUTTONS_H__ */
