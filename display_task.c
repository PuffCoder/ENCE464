/*
 * display_task.c
 *
 *  Created on: 1/08/2023
 *      Author: jbs91
 *
 *      task for updating the display with text from otehr tasks
 */

//INCLUDES ----------------------------------------------------
#include "display_task.h"

#include <string.h>
#include <stdio.h>

#include "drivers/OrbitOLED/OrbitOLEDInterface.h"
#include "freeRTOS.h"
#include "task.h"
#include "priorities.h"
#include "queue.h"

//CONSTANTS----------------------------------------------------
#define DISPLAY_QUEUE_SIZE 10
#define DISPLAY_ITEM_SIZE sizeof(uint32_t)
#define DISPLAY_STACK_SIZE 200

//STATICS AND GLOBALS------------------------------------------------------

QueueHandle_t g_MeasHeightDisplayQueue;
QueueHandle_t g_MeasYawDisplayQueue;
QueueHandle_t g_TargHeightDisplayQueue;
QueueHandle_t g_TargYawDisplayQueue;

//LOCAL FUNCTION PTs-------------------------------------------

static void display_task(void *pvParameters);
static void clear_display(void);
static uint32_t convert_to_height(uint32_t adc_val, uint32_t ground);

//FUNCTIONS----------------------------------------------------

//clears the booster board display
void clear_display(void)
{

    OLEDStringDraw("                    ",1,0);
    OLEDStringDraw("                    ",1,1);
    OLEDStringDraw("                    ",1,2);
    OLEDStringDraw("                    ",1,3);
}

//calibrates the display to display the raw ADC values in a more readable way
uint32_t convert_to_height(uint32_t adc_val, uint32_t ground)
{
    if (adc_val > ground)
    {
        return 0;
    } else {
        return (ground - adc_val) ;
    }
}


//main function to implement the LCD display task
void display_task(void *pvParameters)
{

    //variables
    int32_t recieved_message;
    char* display_output[16];

    static uint32_t curr_Meas_height = 0;
    static uint32_t curr_Targ_height = 0;
    static int32_t curr_Meas_yaw = 0;
    static int32_t curr_Targ_yaw = 0;

    //send initial display message
    clear_display();
    OLEDStringDraw("     Meas  Targ",1,0);
    OLEDStringDraw("H:   000   000",1,1);
    OLEDStringDraw("Y:   000   000",1,2);

    //main loop for task
    while(1)
    {
        int first = 1;
        uint32_t ground_ADC;

        //update measured height
        if (xQueueReceive(g_MeasHeightDisplayQueue, &recieved_message, 0) == pdPASS) {

            curr_Meas_height = recieved_message;

            //calibrate the raw ADC values
            if (first) {

                if (curr_Targ_height > 0){

                    first = 0;
                } else {

                    ground_ADC = curr_Meas_height;
                    ground_ADC += 30;
                }
            }

            //TODO figure out why these two functions five a weird warning
            snprintf(&display_output, sizeof(display_output), "H:   %.4lu  %.3lu ", convert_to_height(curr_Meas_height, ground_ADC), curr_Targ_height);
            OLEDStringDraw(&display_output,1,1);
        }

        //update target height
        if (xQueueReceive(g_TargHeightDisplayQueue, &recieved_message, 0) == pdPASS) {

            curr_Targ_height = recieved_message;

            //TODO figure out why these two functions five a weird warning
            snprintf(&display_output, sizeof(display_output), "H:   %.4lu  %.3lu ", convert_to_height(curr_Meas_height, ground_ADC), curr_Targ_height);
            OLEDStringDraw(&display_output,1,1);
        }

        //update recieved yaw
        if (xQueueReceive(g_MeasYawDisplayQueue, &recieved_message, 0) == pdPASS) {

            curr_Meas_yaw = recieved_message;

            //TODO figure out why these two functions five a weird warning
            if (curr_Meas_yaw < 0) {
                snprintf(&display_output, sizeof(display_output), "Y:   %.3ld  %.3ld ", curr_Meas_yaw, curr_Targ_yaw);
            } else {
                snprintf(&display_output, sizeof(display_output), "Y:    %.3ld  %.3ld ", curr_Meas_yaw, curr_Targ_yaw);
            }
            OLEDStringDraw(&display_output,1,2);
        }

        //update target yaw
        if (xQueueReceive(g_TargYawDisplayQueue, &recieved_message, 0) == pdPASS) {

            curr_Targ_yaw = recieved_message;

            //TODO figure out why these two functions five a weird warning
            if (curr_Meas_yaw < 0) {
                snprintf(&display_output, sizeof(display_output), "Y:   %.3ld  %.3ld ", curr_Meas_yaw, curr_Targ_yaw);
            } else {
                snprintf(&display_output, sizeof(display_output), "Y:    %.3ld  %.3ld ", curr_Meas_yaw, curr_Targ_yaw);
            }
            OLEDStringDraw(&display_output,1,2);
        }
    }
}


//function to initialise the LCD display and other features needed for the display task
uint32_t init_display(void)
{

    //initialise the display peripheral
    OLEDInitialise();

    //send inital message
    clear_display();
    OLEDStringDraw("Display Initialised",1,0);

    //setup queues
    g_MeasHeightDisplayQueue = xQueueCreate(DISPLAY_QUEUE_SIZE, DISPLAY_ITEM_SIZE);
    g_MeasYawDisplayQueue = xQueueCreate(DISPLAY_QUEUE_SIZE, DISPLAY_ITEM_SIZE);
    g_TargHeightDisplayQueue = xQueueCreate(DISPLAY_QUEUE_SIZE, DISPLAY_ITEM_SIZE);
    g_TargYawDisplayQueue = xQueueCreate(DISPLAY_QUEUE_SIZE, DISPLAY_ITEM_SIZE);

    //create the task
    if(xTaskCreate(display_task, (const portCHAR *)"LED", DISPLAY_STACK_SIZE, NULL,
                   tskIDLE_PRIORITY + PRIORITY_DISPLAY_TASK, NULL) != pdTRUE)
    {
        return(1);
    }

    //only reaches here if task fails
    return(0);
}

