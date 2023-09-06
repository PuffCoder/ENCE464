/*
 * control_task.c
 *
 *  Takes inputs from queues from the display task and ADC tasks
 *  and managed the PWM task to respond appropriately.
 *
 *  Created on: 17/08/2023
 *      Author: Jonny Smith
 */


//INCLUDES ----------------------------------------------------
#include "control_task.h"

#include <stdio.h>

#include "freeRTOS.h"
#include "task.h"
#include "priorities.h"
#include "queue.h"
#include "driverlib/timer.h"
#include "driverlib/sysctl.h"
#include "inc/hw_memmap.h"
#include "semphr.h"
#include "utils/uartstdio.h"

//CONSTANTS----------------------------------------------------
#define CONTROL_QUEUE_SIZE 10
#define CONTROL_ITEM_SIZE sizeof(uint32_t)
#define CONTROL_STACK_SIZE 200

#define PROPORTIONAL_GAIN 0.1
#define DERIVATIVE_GAIN 0
#define INTERGRAL_GAIN 0.01e-3
#define PROPORTIONAL_GAIN_YAW 1
#define DERIVATIVE_GAIN_YAW 0
#define INTERGRAL_GAIN_YAW 0.2e-3

#define TIME_PER_TICK 12.5e-6 //ms

//STATICS AND GLOBALS------------------------------------------------------

QueueHandle_t g_MeasHeightControlQueue;
QueueHandle_t g_MeasYawControlQueue;
QueueHandle_t g_TargHeightControlQueue;
QueueHandle_t g_TargYawControlQueue;

extern xSemaphoreHandle g_pUARTSemaphore;
extern xSemaphoreHandle g_ControlSemaphore;
extern xSemaphoreHandle g_ADCSemaphore;

extern QueueHandle_t Q_tailDuty;
extern QueueHandle_t Q_mainDuty;

static uint32_t heights_array[11] = {

    0,
    100,
    200,
    300,
    400,
    500,
    600,
    700,
    800,
    900,
    1000
};

static int32_t yaws_array[24] = {

0,
15,
30,
45,
60,
75,
90,
105,
120,
135,
150,
165,
179,
-165,
-150,
-135,
-120,
-105,
-90,
-75,
-60,
-45,
-30,
-15

};

//LOCAL FUNCTION PTs-------------------------------------------

static void  control_task(void *pvParameters);
static int32_t calc_proportional_gain(int32_t error);
static int32_t calc_derivative_gain(int32_t error, uint32_t time_step);
static int32_t calc_intergral_gain(int32_t error, uint32_t time_step);
static int32_t calc_proportional_gain_yaw(int32_t error);
static int32_t calc_derivative_gain_yaw(int32_t error, uint32_t time_step);
static int32_t calc_intergral_gain_yaw(int32_t error, uint32_t time_step);
static void check_valid_pwm(int32_t* height_pwm);
static uint32_t convert_to_height(uint32_t adc_val, uint32_t ground);

//FUNCTIONS----------------------------------------------------

//used to calibrate the raw ADC values to something usable by the control system
uint32_t convert_to_height(uint32_t adc_val, uint32_t ground)
{
    if (adc_val > ground)
    {
        return 0;
    } else {
        return (ground - adc_val) ;
    }
}

//calculates the proportional gain for the controller
int32_t calc_proportional_gain(int32_t error)
{

    return PROPORTIONAL_GAIN * error;

}

//calculates the derivative gain for the controller
int32_t calc_derivative_gain(int32_t error, uint32_t time_step)

{

    static int32_t last_error = 0;
    int32_t derivative = (last_error - error) / time_step;
    last_error = error;
    return DERIVATIVE_GAIN * derivative;


}

//calculates the intergral gain for the controller
int32_t calc_intergral_gain(int32_t error, uint32_t time_step)
{
    static int32_t total_intergral = 0;
    if (abs(error) < 20) {

        total_intergral = 0;
    }
    total_intergral += error * time_step;
    return INTERGRAL_GAIN * total_intergral;

}

//calculates the proportional gain for the controller
int32_t calc_proportional_gain_yaw(int32_t error)
{

    return PROPORTIONAL_GAIN_YAW * error;

}

//calculates the derivative gain for the controller
int32_t calc_derivative_gain_yaw(int32_t error, uint32_t time_step)

{

    static int32_t last_error = 0;
    int32_t derivative = (last_error - error) / time_step;
    last_error = error;
    return DERIVATIVE_GAIN_YAW * derivative;


}

//calculates the intergral gain for the controller
int32_t calc_intergral_gain_yaw(int32_t error, uint32_t time_step)
{
    static int32_t total_intergral = 0;
    if (abs(error) < 2) {

        total_intergral = 0;
    }
    total_intergral += error * time_step;
    return INTERGRAL_GAIN_YAW * total_intergral;

}

//checks that the pwm is between 0 and 99
void check_valid_pwm(int32_t* height_pwm)
{
    if (*height_pwm < 0) {

        *height_pwm = 0;

    } else if (*height_pwm > 99) {

        *height_pwm = 99;

    }

}


//main loop for the control task
void control_task(void *pvParameters)
{   
    //setup timer
    static uint32_t last_time = 1000000000;
    TimerLoadSet(TIMER0_BASE, TIMER_A, last_time);

    //init variables
    static uint32_t curr_Meas_height;
    static uint32_t curr_Targ_height;
    static int32_t curr_Meas_yaw;
    static uint32_t curr_Targ_yaw;
    static int32_t height_pwm;
    uint32_t ground_ADC;
    int first = 1;

    while(1){
        
        //sephamore to sync up control system with ADC
        if ((xSemaphoreTake(g_ControlSemaphore, 1)) == pdTRUE) {

            //get current values
            xQueueReceive(g_MeasHeightControlQueue, &curr_Meas_height, 0);
            xQueueReceive(g_MeasYawControlQueue, &curr_Meas_yaw, 0);
            xQueueReceive(g_TargHeightControlQueue, &curr_Targ_height, 0);
            xQueueReceive(g_TargYawControlQueue, &curr_Targ_yaw, 0);

            //calibrate the raw ADC values into something usable
            if (first) {

                if (curr_Targ_height > 0){

                    first = 0;
                } else {

                    ground_ADC = curr_Meas_height;
                }
            }

            //determine how long since last control task execution
            uint32_t current_time;
            current_time = TimerValueGet(TIMER0_BASE, TIMER_A);
            uint32_t time_step = last_time - current_time;
            time_step = time_step * TIME_PER_TICK;
            last_time = current_time;

            //make sure clock doesnt overflow
            if (last_time < 400000000) {

                last_time = 1000000000;
                TimerLoadSet(TIMER0_BASE, TIMER_A, last_time);

            }

            //calc error
            int32_t error = heights_array[curr_Targ_height] - convert_to_height(curr_Meas_height, ground_ADC);

            //add offset to the pwm
            height_pwm = 50;
            if ((curr_Targ_height == 0) && (error < 10)) { //make sure its not on teh ground

                height_pwm = 0;
            }

            // cal gains and pwm
            height_pwm += calc_proportional_gain(error);
            height_pwm += calc_derivative_gain(error, time_step);
            height_pwm += calc_intergral_gain(error, time_step);

            check_valid_pwm(&height_pwm);

            //semd pwm
            if(xQueueSend(Q_mainDuty, &height_pwm, portMAX_DELAY) !=
                   pdPASS)
            {
                //
                // Error. The queue should never be full. If so print the
                // error message on UART and wait for ever.
                //
                xSemaphoreTake(g_pUARTSemaphore, portMAX_DELAY);
                UARTprintf("\nQueue full. This should never happen.\n");
                xSemaphoreGive(g_pUARTSemaphore);
                while(1)
                {
                }
            }

            //-------------------
            //yaw
            //-------------------

            //calculate errors
            int32_t y_error1 = 0;
            int32_t y_error2 = 0;
            int32_t y_error = 0;
            y_error1 = yaws_array[curr_Targ_yaw] -curr_Meas_yaw;
            y_error2 = yaws_array[curr_Targ_yaw] -(curr_Meas_yaw + 360);

            //check which way around teh circle is smaller
            if (abs(y_error1) < abs(y_error2) ) {

                y_error = y_error1;
            } else {

                y_error = y_error2;
            }
            y_error = y_error * -1;

            //add offset to pwm
            int32_t yaw_pwm = 40;
            if ((curr_Targ_height == 0) && (error < 10)) {

                yaw_pwm = 0;
            }

            //calc control values
            yaw_pwm += calc_proportional_gain_yaw(y_error);
            yaw_pwm += calc_derivative_gain_yaw(y_error, time_step);
            yaw_pwm += calc_intergral_gain_yaw(y_error, time_step);

            check_valid_pwm(&yaw_pwm);
            if (yaw_pwm > 85) {
                yaw_pwm = 85;
            }
            
            //send pwm
            if(xQueueSend(Q_tailDuty, &yaw_pwm, portMAX_DELAY) !=
                               pdPASS)
            {
                //
                // Error. The queue should never be full. If so print the
                // error message on UART and wait for ever.
                //
                xSemaphoreTake(g_pUARTSemaphore, portMAX_DELAY);
                UARTprintf("\nQueue full. This should never happen.\n");
                xSemaphoreGive(g_pUARTSemaphore);

                while(1)
                {
                }
            }

            //allow adc to run again
            xSemaphoreGive(g_ADCSemaphore);
            vTaskDelay(1);

        }


    }
}

//initialises the control task
uint32_t init_control(void)
{

    //setup timer
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);

    //wait for module to be rdy
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_TIMER0)){}
    TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);
    TimerEnable(TIMER0_BASE, TIMER_A);

    //setup queues
    g_MeasHeightControlQueue = xQueueCreate(CONTROL_QUEUE_SIZE, CONTROL_ITEM_SIZE);
    g_MeasYawControlQueue = xQueueCreate(CONTROL_QUEUE_SIZE, CONTROL_ITEM_SIZE);
    g_TargHeightControlQueue = xQueueCreate(CONTROL_QUEUE_SIZE, CONTROL_ITEM_SIZE);
    g_TargYawControlQueue = xQueueCreate(CONTROL_QUEUE_SIZE, CONTROL_ITEM_SIZE);

    //create task
    if(xTaskCreate(control_task, (const portCHAR *)"CONTROL", CONTROL_STACK_SIZE, NULL,
                   tskIDLE_PRIORITY + PRIORITY_CONTROL_TASK, NULL) != pdTRUE) {

        return 1;
    }

    return 0;

}

