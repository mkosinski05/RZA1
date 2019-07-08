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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

/* OS abstraction specific API header */
#include "r_os_abstraction_api.h"
#include "queue.h"

#include "r_jcu.h"
#include "r_jcu_typedef.h"

#include "dskManager.h"
#include "r_fatfs_abstraction.h"
#include "ff.h"

#include "r_vdc_portsetting.h"
#include "r_rvapi_header.h"
#include "r_display_init.h"

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

#define MAX_TEST_FILE_SIZE        (1024.0f * 1024.0f * 1000.0f)
#define MAX_BUFFER_SIZE            (256 * 1024)

semaphore_t   gs_event_from_JPEG_fin;  /* event_t */
event_t   gs_event_media_play;  /* event_t */
event_t   gs_event_media_stop;  /* event_t */
QueueHandle_t  g_qAudioTrackInfo;

static void task_mjpeg_player ( void );
static int32_t r_jpeg_read_file ( int file, uint8_t *buf, uint32_t file_size );
static void  r_jpeg_decode ( uint8_t* inbuf, uint8_t* outbuf );

CMyGUI::CMyGUI(
    eC_Value x, eC_Value y,
    eC_Value width, eC_Value height,
    ObjectHandle_t eID) :
    CStreamRuntimeGUI(x, y, width, height, eID)

{

	m_pkProgressBar = static_cast<CGUIProgressBar*>(GETGUI.GetObjectByID(AID_PROGRESSBAR_1));

	R_OS_CreateSemaphore(gs_event_from_JPEG_fin, 1 );  /* R_OS_CreateEvent */

	R_OS_CreateEvent( &gs_event_media_play );
	R_OS_CreateEvent( &gs_event_media_stop );

	R_OS_ResetEvent( &gs_event_media_play );
	R_OS_ResetEvent( &gs_event_media_stop );

	// Create MJPEG Task
	R_OS_CreateTask("MJPEG Player",(os_task_code_t)task_mjpeg_player, NULL, R_OS_ABSTRACTION_PRV_DEFAULT_STACK_SIZE, TASK_GRAPHICS_TASK_PRI+5);


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
	if ( kAPI == "media_start" ) {
		R_OS_SetEvent( &gs_event_media_play );
	} else if ( kAPI == "media_stop" ) {
		R_OS_SetEvent( &gs_event_media_stop );
	}
    return true;
}

void CMyGUI::DoAnimate( const eC_Value &vTimers) {
	uint32_t track;

	if ( pdPASS == xQueueReceive ( g_qAudioTrackInfo, &track, 0 )) {
		/* Set the Progress range from 0 -100 */
		if ( NULL != m_pkProgressBar ) {

			m_pkProgressBar->SetValue(track);
			m_pkProgressBar->InvalidateArea();

		}
	}

}


#define     DISP_AREA_HW                (288u)
#define     DISP_AREA_VW                (154u)
#define     VIDEO_BUFFER_STRIDE         (((DISP_AREA_HW * 2u) + 31u) & ~31u)
#define     VIDEO_BUFFER_HEIGHT         (DISP_AREA_VW)
#define 	VIDEO_BUFFER_BYTE_PER_PIXEL  (2u)
extern uint8_t video_buffer1[];
uint8_t jpeg_buffer[1024*32] __attribute__ ((section(".VRAM_SECTION0")));



static int32_t r_jpeg_read_file ( int file, uint8_t *buf, uint32_t file_size ) {
	int ret = 0;
	int32_t chunk_size;
	int32_t bytes_remaining;
	int32_t bytes_done = 0;

	bytes_done = 0;
	bytes_remaining = file_size;

	do {
			if (bytes_remaining > MAX_BUFFER_SIZE)
			{
				chunk_size = MAX_BUFFER_SIZE;
			}
			else
			{
				chunk_size = bytes_remaining;
			}

			ret = read(file, buf, chunk_size);
			//printf("Bytes Read :\t%d\n", ret);
			if ( ret > 0 ) {
				bytes_done += ret;
				bytes_remaining -= ret;
				buf += ret;
			}
	}
	while((bytes_remaining > 0) && (ret >= 0));

	return bytes_done;
}


void task_mjpeg_player ( void ) {

	int ret = 0;

	int	pEntry = NULL;
	char chDrive = 'A';
	char imageFileName[] ="A:\\Renesas.jpg";

	uint32_t FrameSize = 0;
	float FrameRate = 0.0;


	// Mount all Devices
	if ( 0 < dskMountAllDevices() ) {
		if ( NULL == dskGetDrive(chDrive))
			printf("Mount Failed\n");
	}


	while ( 1 ) {

		// Wait for Play Request
		R_OS_WaitForEvent( &gs_event_media_play, R_OS_ABSTRACTION_PRV_EV_WAIT_INFINITE );
		R_OS_ResetEvent( &gs_event_media_play );

		pEntry = open( imageFileName, O_RDWR, _IONBF );

		if ( NULL != pEntry ) {

			ret = read ( pEntry, &FrameRate, sizeof(FrameRate));
			printf("Frame Rate :\t%f\n", FrameRate);
#if 0
			ret = read ( pEntry, &NumOfFrames, sizeof(NumOfFrames));
			printf("Number of Frames :\t%d\n", NumOfFrames);
			FrameNumber = 0;
#endif


			while ( ret > 0 ) {

				if ( R_OS_EventState( &gs_event_media_stop ) == EV_SET ) {
					R_OS_ResetEvent( &gs_event_media_stop );
					break;
				}


				ret = read ( pEntry, &FrameSize, sizeof(FrameSize));
				printf("Frame Size :\t%d\n", FrameSize);

				ret = r_jpeg_read_file ( pEntry, &jpeg_buffer[0], FrameSize);
				if ( ret > 0 ) {

					// Start Decoder
					r_jpeg_decode(jpeg_buffer, video_buffer1);

					// Set to 30 fps
					R_OS_TaskSleep(33);
				}
			}

			close(pEntry);

		}

	}
}

static void  r_jpeg_decode ( uint8_t* inbuf, uint8_t *outbuf )
{
    uintptr_t        physical_address_of_JPEG;
    uintptr_t        physical_address_of_RAW;

    /* physical_address_of_JPEG = ... */
    physical_address_of_JPEG = (uintptr_t) inbuf;

    /* physical_address_of_RAW = ... */
    physical_address_of_RAW  = (uintptr_t) outbuf;
    physical_address_of_RAW &= (uintptr_t) ~(JCU_BUFFER_ALIGNMENT - 1);

    /* Calls JCU API */
    {
        jcu_decode_param_t        decode;
        jcu_buffer_param_t        buffer;

        buffer.source.swapSetting       = JCU_SWAP_LONG_WORD_AND_WORD_AND_BYTE;
        buffer.source.address           = (uint32_t*) physical_address_of_JPEG;
        buffer.destination.address      = (uint32_t*) physical_address_of_RAW;
        buffer.lineOffset               = (int16_t)( DISP_AREA_HW );
        { /* RGB565 */
            decode.decodeFormat            = JCU_OUTPUT_YCbCr422;
            buffer.destination.swapSetting = JCU_SWAP_BYTE;
            decode.outputCbCrOffset        = JCU_CBCR_OFFSET_128;
        }
        decode.alpha                 = 0;
        decode.horizontalSubSampling = JCU_SUB_SAMPLING_1_1;
        decode.verticalSubSampling   = JCU_SUB_SAMPLING_1_1;

        R_JCU_Initialize( NULL );
        R_JCU_SelectCodec( JCU_DECODE );
        R_JCU_SetDecodeParam( &decode,  &buffer );

        R_OS_WaitForSemaphore( gs_event_from_JPEG_fin, 0 ); /* R_OSM_ResetEvent */

        R_JCU_StartAsync( (r_co_function_t) R_OS_ReleaseSemaphore, (void*) &gs_event_from_JPEG_fin ); /* R_OS_SetEvent */
        R_OS_WaitForSemaphore( gs_event_from_JPEG_fin,  R_OS_ABSTRACTION_PRV_EV_WAIT_INFINITE ); /* R_OS_WaitForEvent */
    }


    {
        R_OS_WaitForSemaphore( gs_event_from_JPEG_fin,  0 ); /* R_OS_ResetEvent */

        R_OS_WaitForSemaphore( gs_event_from_JPEG_fin,  R_OS_ABSTRACTION_PRV_EV_WAIT_INFINITE ); /* R_OS_WaitForEvent */

        R_RVAPI_GraphChangeSurfaceVDC( VDC_CHANNEL_0,  VDC_LAYER_ID_0_RD, (void*) outbuf );
    }


    {
        R_OS_WaitForSemaphore( gs_event_from_JPEG_fin,  0 ); /* R_OS_ResetEvent */

        R_JCU_TerminateAsync( (r_co_function_t) R_OS_ReleaseSemaphore,(void*) &gs_event_from_JPEG_fin ); /* R_OS_SetEvent */
        R_OS_WaitForSemaphore( gs_event_from_JPEG_fin,  R_OS_ABSTRACTION_PRV_EV_WAIT_INFINITE ); /* R_OS_WaitForEvent */
    }

} /* End of function R_TOUCH_draw_JPEG_cursor() */
