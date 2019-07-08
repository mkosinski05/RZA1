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
#define USB_ENABLED	1
#ifdef GFX_USE_EGML
#include "GfxWrapeGML.h"
#endif

extern "C"
{
/* Dependencies */
#include <stdbool.h>
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



CMyGUI::CMyGUI(
    eC_Value x, eC_Value y,
    eC_Value width, eC_Value height,
    ObjectHandle_t eID) :
    CStreamRuntimeGUI(x, y, width, height, eID)

{
	// Get pointer to ComboBox
	m_pComboBox =  dynamic_cast<CGUIComboBox*>(GETGUI.GetObjectByID(AID_COMBOBOX_1));


	const eC_Char* kContent[] = { "Earth", "Mars", "Venus", "Jupiter", "Saturn" };
	for(int i=0;i<5;i++)
	{
	    CGUIListItem * pListItem =
	    		new CGUIListItem(NULL, 0, 0, eC_FromInt(80), eC_FromInt(20), kContent[i] );
	    // Modify the newly created item's label
	    pListItem->GetLabel()->SetAligned(CGUIText::V_CENTERED);
	    pListItem->GetLabel()->SetTextColor(0xff000000, 0xffffffff, 0xff000000, 0xffffffff);
	    // Add it to the ComboBox
	    m_pComboBox->AddItem(pListItem);
	}

	// Set some visualization parameters. (e.g. Colors and Images)
	m_pComboBox->SetItemSelectedColor(0xff0000cc);
	m_pComboBox->SetHeaderButtonImages(
	    IMG_STDCTRL_IMGBTN_STANDARD,
	    IMG_STDCTRL_IMGBTN_PRESSED,IMG_STDCTRL_IMGBTN_HIGHLIGHTED,
	    IMG_STDCTRL_IMGBTN_GRAYED_OUT,IMG_STDCTRL_IMGBTN_FOCUSED);
	m_pComboBox->GetHeader()->SetInputFieldImages(
	    IMG_STDCTRL_INPUTFIELD_STANDARD,
	    IMG_STDCTRL_INPUTFIELD_HIGHLIGHTED,
	    IMG_STDCTRL_INPUTFIELD_FOCUSSED,
	    IMG_STDCTRL_INPUTFIELD_GRAYEDOUT);
	// Select an item by index
	m_pComboBox->SetSelection(2);
	// Make the comboxbox-header editable, so that users can enter a search-string
	//m_pComboBox->SetHeaderEditable(true);



}

CMyGUI::~CMyGUI()
{
    // Add application specific de-initialization here if necessary


	delete m_pComboBox;
	m_pComboBox = NULL;



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





