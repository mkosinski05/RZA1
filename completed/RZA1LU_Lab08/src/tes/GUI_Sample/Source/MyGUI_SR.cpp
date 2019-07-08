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

#include "GUI.h"
#include "GUIButton.h"
#include "GUITrace.h"


#include "UserConfig.h"

#ifdef GFX_USE_EGML
#include "GfxWrapeGML.h"
#endif

#include "StreamRuntimeConfig.h"
#include "GUIFramerate.h"

extern "C"
{
#include <stdbool.h>
#include "r_typedefs.h"
#include "r_led_drv_api.h"
#include "r_os_abstraction_api.h"
#include "r_compiler_abstraction_api.h"
#include "iodefine_cfg.h"

#include "r_led_drv_api.h"


/* Dependencies */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include "spibsc_iobitmask.h"
#include "cpg_iobitmask.h"
#include "gpio_iobitmask.h"
extern int16_t gui_hid_mouse_init ( QueueHandle_t msg );
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



CMyGUI::CMyGUI(
    eC_Value x, eC_Value y,
    eC_Value width, eC_Value height,
    ObjectHandle_t eID) :
    CStreamRuntimeGUI(x, y, width, height, eID)

{

	// Create FreeRTOS message Queue
	m_USBMouseQ = xQueueCreate( 100, sizeof(usbM_t) );
	// Initialize USB Mouse
	gui_hid_mouse_init( m_USBMouseQ );
}

CMyGUI::~CMyGUI()
{
    // Add application specific de-initialization here if necessary

}


eC_Bool CMyGUI::DoDrag(const eC_Value& vDeltaX, const eC_Value& vDeltaY, const eC_Value& vAbsX, const eC_Value& vAbsY ) {
	if ( vAbsY > 50) {
		usbMouseMsg.left_button = false;
		usbMouseMsg.right_button = false;
		usbMouseMsg.xPos = vDeltaX;
		usbMouseMsg.yPos = vDeltaY;
		xQueueSend( m_USBMouseQ, (const void *)&usbMouseMsg, 0 );
	}

	return true;
}


void CMyGUI::OnNotification(const CGUIValue& kObservedValue, const CGUIObject* const pkUpdatedObject, const eC_UInt uiX, const eC_UInt uiY)
{
    if (NULL != pkUpdatedObject)
    {

    }
}


eC_Bool CMyGUI::CallApplicationAPI(const eC_String& kAPI, const eC_String& kParam)
{

	 if (kAPI == "leftClick") {

		usbMouseMsg.left_button = true;
		usbMouseMsg.right_button = false;
		usbMouseMsg.xPos = 0;
		usbMouseMsg.yPos = 0;
		xQueueSend( m_USBMouseQ, (const void *)&usbMouseMsg, 0 );

	 }

    return true;
}




