/*
* Copyright (C) 2004 TES Electronic Solutions GmbH,
* All Rights Reserved.
* This source code and any compilation or derivative thereof is the
* proprietary information of TES Electronic Solutions GmbH
* and is confidential in nature.
* Under no circumstances is this software to be exposed to or placed
* under an Open Source License of any type without the expressed
* written permission of TES Electronic Solutions GmbH
*
*############################################################
*/

/******************************************************************************
*   PROJECT:        Guiliani
*******************************************************************************
*
*    MODULE:        MyGUI_SR.cpp
*
*    Archive:       $URL: https://10.25.129.51:3690/svn/GSE/branches/Releases/1.0_Guiliani_2.1/StreamRuntime/src/MyGUI_SR.cpp $
*
*    Date created:  2005
*
*
*
*    Author:        JRE
*
*******************************************************************************
*   MODIFICATIONS
*******************************************************************************
*    ID
*    --------------------------------------------------------------------------
*    $Id: MyGUI_SR.cpp 2159 2014-11-26 15:36:46Z christian.euler $
*
******************************************************************************/
#include "MyGUI_SR.h"




#include "UserConfig.h"

#ifdef GFX_USE_EGML
#include "GfxWrapeGML.h"
#endif



extern "C"
{
/* Dependencies */
#include <stdbool.h>
#include "r_led_drv_api.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include "r_adc.h"
}

#if __ICCARM__ == 1
extern "C"
{
    int open(const char *filename, int amode);
    int close(int handle);
}
#endif


// LAST INCLUDE!!
#include "GUIMemLeakWatcher.h"


#define ADC_MAX_VALUE	4095

CMyGUI::CMyGUI(
    eC_Value x, eC_Value y,
    eC_Value width, eC_Value height,
    ObjectHandle_t eID) :
    CStreamRuntimeGUI(x, y, width, height, eID)

{
	pkTextField = static_cast<CGUITextField*>(GETGUI.GetObjectByID(AID_TEXTFIELD_1));
	pkProgressBar = static_cast<CGUIProgressBar*>(GETGUI.GetObjectByID(AID_PROGRESSBAR_1));

	// add callback for polling RTC
	GETTIMER.AddAnimationCallback(100, this);
}

CMyGUI::~CMyGUI()
{
    // Add application specific de-initialisation here if necessary

}

void CMyGUI::DoAnimate(const eC_Value &vTimes)
{
	char data_str[32];
	uint32_t data = 0;
	r_adc_cfg adc_settings = {
			ADC_NO_TRIGGER,
			ADC_256TCYC,
			ADC_SINGLE,
			R_ADC_CH2
	};

	R_ADC_Open(&adc_settings);
	R_ADC_Read((uint32_t*)&adc_val, R_ADC_CH2, 1);

	R_ADC_Close(R_ADC_CH2);

	sprintf(data_str, "%d", (uint16_t)adc_val);
	pkTextField->SetLabel(data_str);

	data = (uint32_t)((((float32_t)adc_val)/ADC_MAX_VALUE)*100);

	pkProgressBar->SetValue(data);
	pkProgressBar->InvalidateArea();

}

void CMyGUI::OnNotification(const CGUIValue& kObservedValue, const CGUIObject* const pkUpdatedObject, const eC_UInt uiX, const eC_UInt uiY)
{
    if (NULL != pkUpdatedObject)
    {

    }
}


eC_Bool CMyGUI::CallApplicationAPI(const eC_String& kAPI, const eC_String& kParam)
{
	int_t led_handle = (-1);
	uint16_t led = LED0;

	// Debug print captured Command
	printf( "%s\n", (char*)kAPI.ToASCII_Alloc());

	if (kAPI == "Led0")
	    {

			/* open LED driver */
			led_handle = open( DEVICE_INDENTIFIER "led", O_RDWR);

			/* LED OFF */
			control(led_handle, CTL_SET_LED_OFF, &led);

			R_OS_TaskSleep((adc_val / 2) + 100);

			/* LED ON */
			control(led_handle, CTL_SET_LED_ON, &led);


			close(led_handle);
	    }
    return true;
}



void CMyGUI::NotifyOfDestruction(const CGUIObject* pDestructedObject)
{

}


