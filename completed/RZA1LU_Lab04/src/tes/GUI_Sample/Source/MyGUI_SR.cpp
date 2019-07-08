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


#include "UserConfig.h"

#ifdef GFX_USE_EGML
#include "GfxWrapeGML.h"
#endif



extern "C"
{
/* Dependencies */
#include <stdbool.h>
#include "r_switch_driver.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>


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

FILE io_switch;
void my_initialise_switch_monitor_task (void);

CMyGUI::CMyGUI(
    eC_Value x, eC_Value y,
    eC_Value width, eC_Value height,
    ObjectHandle_t eID) :
    CStreamRuntimeGUI(x, y, width, height, eID)

{
	my_initialise_switch_monitor_task( );
}

CMyGUI::~CMyGUI()
{
    // Add application specific de-initialisation here if necessary

}


void CMyGUI::OnNotification(const CGUIValue& kObservedValue, const CGUIObject* const pkUpdatedObject, const eC_UInt uiX, const eC_UInt uiY)
{
    if (NULL != pkUpdatedObject)
    {

    }
}


eC_Bool CMyGUI::CallApplicationAPI(const eC_String& kAPI, const eC_String& kParam)
{

    return true;
}




static event_t gs_switch_event;

static void switch_pressed(void);
/***********************************************************************************************************************
 * Function Name: switch_task
 * Description  : Switch monitoring task
 * Arguments    : void *parameters
 * Return Value : none
 ***********************************************************************************************************************/
static void my_switch_task (void *parameters)
{
    UNUSED_PARAM(parameters);
    CGUIButton* m_pkButtonParent;

    R_OS_CreateEvent(&gs_switch_event);

    /* endless loop */
    while (1)
    {
        R_OS_WaitForEvent(&gs_switch_event, R_OS_ABSTRACTION_PRV_EV_WAIT_INFINITE);
        m_pkButtonParent = static_cast<CGUIButton*>(GETGUI.GetObjectByID(AID_BUTTON_1));
        if ( m_pkButtonParent->IsGrayedOut() ) {
        	m_pkButtonParent->SetGrayedOut(false);
        } else {
        	m_pkButtonParent->SetGrayedOut(true);
        }

    }
}
/***********************************************************************************************************************
 End of function switch_task
 ***********************************************************************************************************************/
/***********************************************************************************************************************
 * Function Name: switch_pressed
 * Description  : Switch pressed callback - this is called from an interrupt routine
 * Arguments    : none
 * Return Value : none
 ***********************************************************************************************************************/
static void my_switch_pressed (void)
{
    /* notify the switch task that the switch has been pressed */
    R_OS_SetEvent(&gs_switch_event);
}
/***********************************************************************************************************************
 End of function switch_pressed
 ***********************************************************************************************************************/

/***********************************************************************************************************************
 * Function Name: initialise_switch_monitor_task
 * Description  : Initialises the switch driver and creates the switch monitor task
 * Arguments    : none
 * Return Value : none
 ***********************************************************************************************************************/
void my_initialise_switch_monitor_task ( void )
{
    os_task_t *p_os_task;

    R_SWITCH_Init(true);
    R_SWITCH_SetPressCallback(my_switch_pressed);

    /* Create a task to monitor the switch */
    p_os_task = R_OS_CreateTask("Switch", my_switch_task, NULL, R_OS_ABSTRACTION_PRV_DEFAULT_STACK_SIZE, TASK_SWITCH_TASK_PRI);

    /* NULL signifies that no task was created by R_OS_CreateTask */
    if (NULL == p_os_task)
    {
        /* Debug message */
    }
}
