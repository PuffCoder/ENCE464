/**
 * @file yaw_task.h
 * @author [Heng Yin] -- hyi32
 * @date [17 Aug 2023]
 * @brief Header for the yaw task module.
 *
 * This file contains the definitions and function prototypes required for the yaw task module.
 * The module is responsible for handling and determining the yaw orientation of the system.
 * 
 * Included in this module are:
 * - An enumerated type representing the yaw direction (Anticlockwise, Still, Clockwise).
 * - A lookup table for determining direction.
 * - Function prototypes for initializing the yaw system, updating the direction, and calculating yaw.
 */

#ifndef __YAW_TASK_H__
#define __YAW_TASK_H__

// ******************************* Yaw Settings *******************************

/** 
 * Lookup table to determine the yaw direction based on the states of the two input pins.
 * This table translates the combined previous and current states of the two pins into 
 * a direction. It is indexed by combining the previous states (two most significant bits) 
 * with the current states (two least significant bits) to get a value between 0 and 15.
 */
static int8_t Dir_List[] = {0, -1, 1, 0, 
                        1, 0, 0, -1, 
                        -1, 0, 0, 1,
                        0, 1, -1, 0};

/** 
 * Enum to represent the direction of the yaw.
 * ANTICLOCKWISE: Indicates the yaw is moving in the anticlockwise direction.
 * STILL: Indicates the yaw is not moving.
 * CLOCKWISE: Indicates the yaw is moving in the clockwise direction.
 */
enum Yaw_Dir {
    CLOCKWISE = -1, 
    STILL, 
    ANTICLOCKWISE
};

// Initializes the Yaw Task.
uint32_t YAWTaskInit(void);

// Sets up the necessary hardware peripherals for yaw detection.
void initialiseYaw(void);

// Initializes yaw reference settings.
void initYawRef(void);

int32_t get_current_yaw(void);


#endif // __YAW_TASK_H__

