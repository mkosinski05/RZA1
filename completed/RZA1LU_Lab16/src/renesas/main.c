/*******************************************************************************
 * DISCLAIMER
 * This software is supplied by Renesas Electronics Corporation and is only
 * intended for use with Renesas products. No other uses are authorized. This
 * software is owned by Renesas Electronics Corporation and is protected under
 * all applicable laws, including copyright laws.
 * THIS SOFTWARE IS PROVIDED "AS IS" AND RENESAS MAKES NO WARRANTIES REGARDING
 * THIS SOFTWARE, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING BUT NOT
 * LIMITED TO WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE
 * AND NON-INFRINGEMENT. ALL SUCH WARRANTIES ARE EXPRESSLY DISCLAIMED.
 * TO THE MAXIMUM EXTENT PERMITTED NOT PROHIBITED BY LAW, NEITHER RENESAS
 * ELECTRONICS CORPORATION NOR ANY OF ITS AFFILIATED COMPANIES SHALL BE LIABLE
 * FOR ANY DIRECT, INDIRECT, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES FOR
 * ANY REASON RELATED TO THIS SOFTWARE, EVEN IF RENESAS OR ITS AFFILIATES HAVE
 * BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
 * Renesas reserves the right, without notice, to make changes to this software
 * and to discontinue the availability of this software. By using this software,
 * you agree to the additional terms and conditions found by accessing the
 * following link:
 * http://www.renesas.com/disclaimer
 * Copyright (C) 2018 Renesas Electronics Corporation. All rights reserved.
 *******************************************************************************/
/*******************************************************************************
 * File Name    : main.c
 * Device(s)    : RZ/A1LU
 * Tool-Chain   : GCC for Renesas RZ
 * OS           : FreeRTOS Version See src/freertos/include/FreeRTOS.h
 * H/W Platform : Stream it! v2 board
 * Description  : WebEngine
 *                This demonstration run on the Renesas Stream it! v2 platform.
 *******************************************************************************/

/* Dependencies */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include "compiler_settings.h"

#include "iodefine_cfg.h"
#include "control.h"
#include "version.h"
#include "dma_if.h"

#include "spibsc_iobitmask.h"
#include "cpg_iobitmask.h"
#include "gpio_iobitmask.h"

/* STDIO devlink driver drivers */
#include "r_led_drv_api.h"
/* End of STDIO devlink driver drivers */

#include "command.h"
#include "r_task_priority.h"

#include "lwipStart.h"
#include "cmd_tcp_shell.h"
#include "webif.h"
#include "main.h"


static st_iostr_t gs_iostr[3];
char gs_default_prompt[] = "REE>";
event_t gs_switch_event;

#if R_SELF_LOAD_MIDDLEWARE_ETHERNET_MODULES
static PTCSH spTcpCmdShell = NULL;
#endif
void R_MSG_WarningConfig(pst_comset_t p_com, char_t *msg);
int_t g_led_handle = ( -1);

extern int_t gui_main(void);
extern int_t g_rtc_handle;

static void my_switch_pressed (void)
{
    /* notify the switch task that the switch has been pressed */
    R_OS_SetEvent(&gs_switch_event);
}

void R_ETH_MiddlewareStart(void)
{

#if R_SELF_LOAD_MIDDLEWARE_ETHERNET_MODULES

    R_OS_TaskSleep(500);

    /* Initialise lwIP - print the network status to the serial port */
    ipStartOnChipController(gs_iostr[0].p_out);

    /* Start a TCP/IP command shell for telnet on port 23 */
    spTcpCmdShell = tcpCmdShellStart(23);

    /* Start Webio web server */
    wsStart(80);
#endif

}

void R_ETH_MiddlewareStop(void)
{
#if R_SELF_LOAD_MIDDLEWARE_ETHERNET_MODULES

    R_OS_TaskSleep(500);

    /* Stop Webio web server */
    wsStop();

    /* Stop a TCP/IP command shell for telnet on port 23 */
    tcpCmdShellStop(spTcpCmdShell);
    spTcpCmdShell = NULL;

    /* Un-initialise lwIP  */
    ipStop();
#endif
}
void R_MSG_WarningConfig(pst_comset_t p_com, char_t *msg)
{
    if(NULL != p_com->p_out)
    {
        fprintf(p_com->p_out,"Warning command disabled\r\n"
                "In configuration file /RZA1_Sample/src/renesas/configuration/application_cfg.h\r\n"
                "%s should be disabled when using this command\r\n", msg);
    }
}

#if R_SELF_BLINK_TASK_CREATION
/***********************************************************************************************************************
 * Function Name: blink_task
 * Description  : LED blinking task
 * Arguments    : void *parameters
 * Return Value : none
 ***********************************************************************************************************************/
static void blink_task (void *parameters)
{
    UNUSED_PARAM(parameters);
    uint16_t led = LED0;

    if (g_led_handle < 0)
    {
        printf("gs_stdio_led_handle led driver failed to OPEN \r\n");
    }

    /* Endless loop */
    while (1)
    {

        control(g_led_handle, CTL_SET_LED_ON, &led);
        R_OS_TaskSleep(300);

        control(g_led_handle, CTL_SET_LED_OFF, &led);
        R_OS_TaskSleep(700);
    }
}
/***********************************************************************************************************************
 End of function blink_task
 ***********************************************************************************************************************/
#endif

/***********************************************************************************************************************
 * Function Name: console_task
 * Description  : Serial console task, connect 115200:8:N:1
 * Arguments    : void *parameters
 * Return Value : none
 ***********************************************************************************************************************/
static void console_task (void *parameters)
{
    (void) parameters;
    static bool_t isrunning = true;

    R_OS_TaskUsesFloatingPoint();

    /* Assign the streams for the serial console */
    gs_iostr[0].p_in = stdin;
    gs_iostr[0].p_out = stdout;

#if R_SELF_LOAD_MIDDLEWARE_USB_HOST_CONTROLLER
    int_t usb0_handle = ( -1);
#endif

    R_SWITCH_Init(true);
	R_SWITCH_SetPressCallback(my_switch_pressed);

	R_OS_CreateEvent(&gs_switch_event);

    /* open resident drivers */
    g_rtc_handle = open( DEVICE_INDENTIFIER "rtc", O_RDONLY);

#if R_SELF_LOAD_MIDDLEWARE_USB_HOST_CONTROLLER
    usb0_handle = open( DEVICE_INDENTIFIER "usb0", O_RDWR);
#endif

    R_ETH_MiddlewareStart();
#if R_SELF_INSERT_APP_GUI
    //vTaskDelay( pdMS_TO_TICKS(1000) );
    while (1) {
    	R_OS_TaskSleep(10000);
    }

#else
    /* Endless loop */
    while (isrunning)
    {
        cmd_console(gs_iostr->p_in, gs_iostr->p_out, gs_default_prompt);
    }
#endif
    R_ETH_MiddlewareStop();

    /* close resident drivers */
    close(g_rtc_handle);

#if R_SELF_LOAD_MIDDLEWARE_USB_HOST_CONTROLLER
    close(usb0_handle);
#endif

}
/***********************************************************************************************************************
 End of function console_task
 ***********************************************************************************************************************/

/***********************************************************************************************************************
 * Function Name: main
 * Description  : Main entry point
 * Arguments    : none
 * Return Value : 0
 ***********************************************************************************************************************/
int_t main (void)
{
    /* OS variables */
    os_task_t *p_os_task;

    /* open LED driver */
    g_led_handle = open( DEVICE_INDENTIFIER "led", O_RDWR);

#if R_SELF_BLINK_TASK_CREATION
    /* Create a task to blink the LED */
    p_os_task = R_OS_CreateTask("Blink", blink_task, NULL, R_OS_ABSTRACTION_PRV_DEFAULT_STACK_SIZE, TASK_BLINK_TASK_PRI);

    /* NULL signifies that no task was created by R_OS_CreateTask */
    if (NULL == p_os_task)
    {
        /* Debug message */
    }
#endif




    /* Need to determine system state is running */
    if (R_OS_GetNumberOfTasks())
    {
        /* At least 1 task is running so scheduler is up */
        /* As an enhancement we should use a status semaphore to determine if the console wished to terminate the application
         * but for now just assume that the console will never stop */

        /* Create a task to run the console */
        p_os_task = R_OS_CreateTask("Console", console_task, NULL, R_OS_ABSTRACTION_PRV_HUGE_STACK_SIZE,
                TASK_CONSOLE_TASK_PRI);

        /* NULL signifies that no task was created by R_OS_CreateTask */
        if (NULL == p_os_task)
        {
            /* Debug message */
            /* R_COMPILER_Nop() */
            ;
        }

        while (1)
        {
        	R_OS_TaskSleep(10000);
        }
    }

    /* Shut down all system resources */
    if (( -1) != g_led_handle)
    {
        close(g_led_handle);
    }

    /* This function should never return */
    while (1)
    {
        /* R_COMPILER_Nop() */
        ;
    }

    return 0;
}
/***********************************************************************************************************************
 End of function main
 ***********************************************************************************************************************/

/***********************************************************************************************************************
 * Function Name: abort
 * Description  : abort handler may be called by libraries
 * Arguments    : none
 * Return Value : 0
 ***********************************************************************************************************************/
void abort (void)
{
    while (1)
    {
        R_COMPILER_Nop();
    }
}
/***********************************************************************************************************************
 End of function abort
 ***********************************************************************************************************************/
