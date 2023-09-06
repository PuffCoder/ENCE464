/*
 * all_buttons.c
 *
 * Sets up and polls the 4 directional buttons on the Tiva Board.
 *
 *  Created on: 5/08/2023
 *      Author: Jamie Thomas
 */

#include <stdbool.h>
#include <stdint.h>
#include "inc/hw_gpio.h"
#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "all_buttons.h"

//*****************************************************************************
//
// Buttons set-up.
//
//*****************************************************************************
button_t button_up = {
      GPIO_PORTE_BASE,
      SYSCTL_PERIPH_GPIOE,
      GPIO_PIN_0,
      GPIO_PIN_TYPE_STD_WPD,
      UP_BUTTON
};

button_t button_right = {
      GPIO_PORTF_BASE,
      SYSCTL_PERIPH_GPIOF,
      GPIO_PIN_0,
      GPIO_PIN_TYPE_STD_WPU,
      RIGHT_BUTTON
};

button_t button_down = {
      GPIO_PORTD_BASE,
      SYSCTL_PERIPH_GPIOD,
      GPIO_PIN_2,
      GPIO_PIN_TYPE_STD_WPD,
      DOWN_BUTTON
};

button_t button_left = {
      GPIO_PORTF_BASE,
      SYSCTL_PERIPH_GPIOF,
      GPIO_PIN_4,
      GPIO_PIN_TYPE_STD_WPU,
      LEFT_BUTTON
};

//*****************************************************************************
//
// Gets the button object based on a number/name input (Name from enum).
//
//*****************************************************************************
button_t getButton(uint8_t buttonNumber)
{
    button_t button = {};
    switch (buttonNumber)
    {
        case UP_BUTTON:
            button = button_up;
            break;
        case RIGHT_BUTTON:
            button = button_right;
            break;
        case DOWN_BUTTON:
            button = button_down;
            break;
        case LEFT_BUTTON:
            button = button_left;
            break;
        default:
            break;
    }

    return button;
}

//*****************************************************************************
//
// Initialise buttons to their given pull-up or pull-down states, and as input.
//
//*****************************************************************************
void ButtonsInit(void)
{
    int i;
    for (i = 0; i < NUMOFBUTTONS; i++)
    {
        button_t button = getButton(i);

        MAP_SysCtlPeripheralEnable(button.periph);

        MAP_GPIODirModeSet(button.base, button.pin, GPIO_DIR_MODE_IN);
        MAP_GPIOPadConfigSet(button.base, button.pin,
                             GPIO_STRENGTH_2MA, button.pull_up);

    }

    // Unlock the right button GPIO port
    HWREG(GPIO_PORTF_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY;
    HWREG(GPIO_PORTF_BASE + GPIO_O_CR) |= 0x01;
    HWREG(GPIO_PORTF_BASE + GPIO_O_LOCK) = 0;

}

//*****************************************************************************
//
// Polls the buttons and gets a boolean value back as to whether it has been
// pushed.
//
//*****************************************************************************
bool ButtonsPoll(uint8_t buttonName)
{

    bool ui8Data;

    button_t button = getButton(buttonName);

    if (button.base)
    {
        ui8Data = MAP_GPIOPinRead(button.base, button.pin);
    }

    if (button.pull_up == GPIO_PIN_TYPE_STD_WPD)
    {
        return !(!ui8Data);
    }
    return !ui8Data;

}



