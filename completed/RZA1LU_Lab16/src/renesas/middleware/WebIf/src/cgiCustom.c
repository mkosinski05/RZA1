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
 * File Name    : webCGI.c
 * Version      : 1.00
 * Description  : Common Gateway Interface custom file handler functions
 ******************************************************************************
 * History      : DD.MM.YYYY Ver. Description
 *              : 04.02.2010 1.00 First Release
 ******************************************************************************/

/******************************************************************************
 WARNING!  IN ACCORDANCE WITH THE USER LICENCE THIS CODE MUST NOT BE CONVEYED
 OR REDISTRIBUTED IN COMBINATION WITH ANY SOFTWARE LICENSED UNDER TERMS THE
 SAME AS OR SIMILAR TO THE GNU GENERAL PUBLIC LICENCE
 ******************************************************************************/

/******************************************************************************
 Includes   <System Includes> , "Project Includes"
******************************************************************************/
#include <ctype.h>
#include <stdio.h>
#include "control.h"
#include "websys.h"
#include "webCGI.h"
#include "r_switch_driver.h"
#include "r_adc.h"
#include "r_os_abstraction_api.h"

/******************************************************************************
 Macro definitions
 ******************************************************************************/

/*****************************************************************************
 Function Macros
 ******************************************************************************/

/*****************************************************************************
 Enumerated Types
 ******************************************************************************/

/*****************************************************************************
 Typedefs
 ******************************************************************************/

/*****************************************************************************
 Constant Data
 ******************************************************************************/
extern event_t gs_switch_event;

int cgiSwitchMonitor (PSESS pSess, PEOFILE pEoFile)
{

	if (R_OS_WaitForEvent(&gs_switch_event, 0)) {
		wi_printf(pSess, "<form>\r\n<input type=\"radio\" value=\"Switch\" checked> <br>\r\n</form>\r\n");
	} else {
		wi_printf(pSess, "<input type=\"radio\" value=\"Switch\" > <br>");
	}

    return (0);
}

int cgiGetADC (PSESS pSess, PEOFILE pEoFile)
{
	uint32_t adc_val = 1;
	r_adc_cfg adc_settings = {
			ADC_NO_TRIGGER,
			ADC_256TCYC,
			ADC_SINGLE,
			R_ADC_CH2
	};

	R_ADC_Open(&adc_settings);
	R_ADC_Read((uint32_t*)&adc_val, R_ADC_CH2, 1);

	R_ADC_Close(R_ADC_CH2);

	/* Format the HTML ADC */
	wi_printf(pSess, "<p>%d</p>\r\n", adc_val);

    return 0;
}



