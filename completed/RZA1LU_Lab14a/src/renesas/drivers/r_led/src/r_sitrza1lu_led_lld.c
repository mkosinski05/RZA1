/******************************************************************************
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
 *******************************************************************************
 * Copyright (C) 2012 Renesas Electronics Corporation. All rights reserved.
 *******************************************************************************
 * File Name    : r_rskrza1h_led_lld.c
 * Description  : LED device driver
 *******************************************************************************
 * History      : DD.MM.YYYY Version Description
 
 *              : 08.12.2017 1.00    First Release
 ******************************************************************************/

/******************************************************************************
 WARNING!  IN ACCORDANCE WITH THE USER LICENCE THIS CODE MUST NOT BE CONVEYED
 OR REDISTRIBUTED IN COMBINATION WITH ANY SOFTWARE LICENSED UNDER TERMS THE
 SAME AS OR SIMILAR TO THE GNU GENERAL PUBLIC LICENCE
 ******************************************************************************/

/******************************************************************************
 Includes   <System Includes> , "Project Includes"
 ******************************************************************************/

/* hardware access includes */
#include "iodefine_typedef.h"
#include "rza_io_regrw.h"

#include "gpio_iodefine.h"
#include "intc_iodefine.h"

#include "gpio_iobitmask.h"
#include "intc_iobitmask.h"
#include "rza_io_regrw.h"
/* end of hardware access includes */

#include "dev_drv.h"
#include "r_devlink_wrapper.h"
#include "r_led_drv_api.h"

/* compiler specific api header */
#include "r_compiler_abstraction_api.h"

/** Register definitions, common services and error codes. */

/******************************************************************************
 Macro definitions
 ******************************************************************************/

/* Comment this line out to turn ON module trace in this file */
#undef _TRACE_ON_

#ifndef _TRACE_ON_
#undef TRACE
#define TRACE_PRV_(x)
#endif

static const st_drv_info_t gs_lld_info =
{
{ ((STDIO_LED_RZ_LLD_VERSION_MAJOR << 16) + STDIO_LED_RZ_LLD_VERSION_MINOR) },
  STDIO_LED_RZ_LLD_BUILD_NUM,
  STDIO_LED_RZ_LLD_DRV_NAME };

#define R_LLD_LED_NUMBER_OF_SUPPORTED_LEDS_PRV (2)

typedef enum ports
{
    P71 = 0,
    IIC2P9,
} e_ports_t;

typedef enum active
{
    HIGH = 0,
    LOW,
} e_active_t;

typedef enum default_state
{
    OFF = 0,
    ON,
} e_default_state_t;

typedef struct
{
    const e_ports_t         port;
    const e_active_t        active_high;
    const e_default_state_t default_state;
} st_r_led_t;

static const struct rskrza1h_lld_led
{
    const st_r_led_t st_x_led;
} gs_st_rskrza1h_lld_led[] =
{
 { {P71, HIGH, OFF} },
 { {IIC2P9, HIGH, OFF} }
};

/*******************************************************************************
 * Function Name: R_LED_InitialiseHwIf
 * Description  : LowLevel driver initialise interface function
 * Arguments    : none
 * Return Value : none
 ******************************************************************************/
void R_LED_InitialiseHwIf(void)
{
    uint16_t count;
    for (count = 0; count < R_LLD_LED_NUMBER_OF_SUPPORTED_LEDS_PRV; count++)
    {
        switch (gs_st_rskrza1h_lld_led[count].st_x_led.port)
        {
            case P71:
            {
                /* ---- P7_8 : D32 LED0 (USER) LED direct connection to IP */
                /* ensures proper access peripheral by casting st_gpio */
                rza_io_reg_write_16(&GPIO.PMC7, 0, GPIO_PMC7_PMC78_SHIFT, GPIO_PMC7_PMC78);

                /* ensures proper access peripheral by casting st_gpio */
                rza_io_reg_write_16(&GPIO.P7, 1, GPIO_P7_P78_SHIFT, GPIO_P7_P78);

                /* ensures proper access peripheral by casting st_gpio */
                rza_io_reg_write_16(&GPIO.PM7, 0, GPIO_PM7_PM78_SHIFT, GPIO_PM7_PM78);

                /* ensures proper access peripheral by casting st_gpio */
                rza_io_reg_write_16(&GPIO.PIPC7, 0, GPIO_PIPC7_PIPC78_SHIFT, GPIO_PIPC7_PIPC78);

                break;
            }
        }
    }
}
/*******************************************************************************
 End of function R_LED_InitialiseHwIf
 ******************************************************************************/


/*******************************************************************************
 * Function Name: R_LED_UninitialiseHwIf
 * Description  : LowLevel driver un-initialise interface function
 * Arguments    : none
 * Return Value : none
 ******************************************************************************/
void R_LED_UninitialiseHwIf(void)
{
    /* currently not needed */

    /* Do Nothing */
    R_COMPILER_Nop();
}
/*******************************************************************************
 End of function R_LED_UninitialiseHwIf
 ******************************************************************************/


/*******************************************************************************
 * Function Name: R_LED_Update
 * Description  : Function to update the LEDs
 * Arguments    : IN  byLedMap - a bit map of the LEDs
 * Return Value : DEVDRV_SUCCESS for success or DEVDRV_ERROR on error
 ******************************************************************************/
int32_t R_LED_Update(int32_t iLedMap)
{
    int32_t res = DEVDRV_ERROR;
    int32_t i_led_map = iLedMap;

    /* Check for the LED on the port pin */
    if (i_led_map & LED0)
    {
        /* Switch the LED on the port pin on */
        rza_io_reg_write_16 (&GPIO.P7, 0, GPIO_P7_P78_SHIFT, GPIO_P7_P78);
        res = DEVDRV_SUCCESS;
    }
    else
    {
        /* Switch the LED on the port pin off */
        rza_io_reg_write_16 (&GPIO.P7, 1, GPIO_P7_P78_SHIFT, GPIO_P7_P78);
        res = DEVDRV_SUCCESS;
    }

    return (res);
}
/******************************************************************************
 End of function  R_LED_Update
 ******************************************************************************/

/*******************************************************************************
 * Function Name: R_LED_ReadLed
 * Description  : reads the status of a single LED directly from the hardware
 * Arguments    : led - selected LED
 * Return Value : status -
 *                     0 led is not set
 *                     1 led is set
 *                     0xFF error
 ******************************************************************************/
uint8_t R_LED_ReadLed(uint32_t led)
{
    uint8_t status = 0xFF;

    /* only LED0 is supported at this time */
    if (LED0 == led)
    {
        /*
         * Use check instead of directly mapping to allow for inverted led (0 = on, 1 = off);
         */
        if (rza_io_reg_read_16 (&GPIO.P7, GPIO_P7_P78_SHIFT, GPIO_P7_P78))
        {
            /* led is set */
            status = 1;
        }
        else
        {
            /* led is not set */
            status = 0;
        }
    }
    return status;
}
/*******************************************************************************
 End of function R_LED_ReadLed
 ******************************************************************************/

/*******************************************************************************
 * Function Name: R_LED_GetVersion
 * Description  :
 * Arguments    : pointer to version structure to report to
 * Return Value : DEVDRV_SUCCESS
 ******************************************************************************/
int32_t R_LED_GetVersion(st_ver_info_t *pinfo)
{
    pinfo->lld.version.sub.major = gs_lld_info.version.sub.major;
    pinfo->lld.version.sub.minor = gs_lld_info.version.sub.minor;
    pinfo->lld.build = gs_lld_info.build;
    pinfo->lld.p_szdriver_name = gs_lld_info.p_szdriver_name;
    return (DEVDRV_SUCCESS);
}
/*******************************************************************************
 End of function R_LED_GetVersion
 ******************************************************************************/

/******************************************************************************
 End  Of File
 ******************************************************************************/
