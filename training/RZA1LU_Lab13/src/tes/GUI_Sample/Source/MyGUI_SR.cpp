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

//#include "StreamRuntimeConfig.h"
//#include "GUIFramerate.h"

extern "C"
{

/* Dependencies */
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include "r_sound.h"
}



// LAST INCLUDE!!
#include "GUIMemLeakWatcher.h"



CMyGUI::CMyGUI(
    eC_Value x, eC_Value y,
    eC_Value width, eC_Value height,
    ObjectHandle_t eID) :
    CStreamRuntimeGUI(x, y, width, height, eID)

{
	m_pkProgressBar = static_cast<CGUIProgressBar*>(GETGUI.GetObjectByID(AID_PROGRESSBAR_1));

	m_qAudioTrackInfo = xQueueCreate(10, sizeof(uint32_t));

	// add callback for polling RTC
	GETTIMER.AddAnimationCallback(25, this);

	R_SOUND_PlaySample_init(m_qAudioTrackInfo);

}

CMyGUI::~CMyGUI()
{
    // Add application specific de-initialization here if necessary
	if ( NULL != m_pkProgressBar ) {
		delete m_pkProgressBar;
		m_pkProgressBar = NULL;
	}

	if ( NULL != m_pPlayButton ) {
		delete m_pPlayButton;
		m_pPlayButton = NULL;
	}
	if ( NULL != m_pStopButton ) {
		delete m_pStopButton;
		m_pStopButton = NULL;
	}
	if ( NULL != m_qAudioTrackInfo ) {
		vQueueDelete ( m_qAudioTrackInfo );
		m_qAudioTrackInfo = NULL;
	}

}


void CMyGUI::DoAnimate( const eC_Value &vTimers) {
	uint32_t track;

	if ( pdPASS == xQueueReceive ( m_qAudioTrackInfo, &track, 0 )) {
		/* Set the Progress range from 0 -100 */
		if ( NULL != m_pkProgressBar ) {

			m_pkProgressBar->SetValue(track);
			m_pkProgressBar->InvalidateArea();

		}
	}

}
void CMyGUI::OnNotification(const CGUIValue& kObservedValue, const CGUIObject* const pkUpdatedObject, const eC_UInt uiX, const eC_UInt uiY)
{
    if (NULL != pkUpdatedObject)
    {

    }
}


eC_Bool CMyGUI::CallApplicationAPI(const eC_String& kAPI, const eC_String& kParam)
{
	if ( "media_play" == kAPI ) {
		char buf[80];
		R_SOUND_PlaySample();


	} else if ( "media_stop" == kAPI ) {
		R_SOUND_StopSample();
	}
    return true;
}








