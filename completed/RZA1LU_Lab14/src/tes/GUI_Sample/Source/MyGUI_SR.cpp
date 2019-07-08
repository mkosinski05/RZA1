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

#include "r_os_abstraction_api.h"
#include "queue.h"

#include "r_devlink_wrapper_cfg.h"
#include "dskManager.h"

#include "r_vdc_portsetting.h"
#include "r_rvapi_header.h"
#include "r_display_init.h"

}

extern uint8_t video_buffer[];
extern uint16_t video_size;
//uint8_t jpg_buffer [ (320 * 240 *2) + 31];

#if __ICCARM__ == 1
extern "C"
{
    int open(const char *filename, int amode);
    int close(int handle);
}
#endif

// LAST INCLUDE!!
#include "GUIMemLeakWatcher.h"

#define WORKING_DRV 'A'
#define ROOT_PATH	"A:\\"
#define MAX_TEST_FILE_SIZE        (1024.0f * 1024.0f * 1000.0f)
#define MAX_BUFFER_SIZE            (256 * 1024)

typedef enum {
	USBH_NORMAL,
	USBH_CAPTURE_FRAMEBUFFER,
	USBH_DISPLAY_IMAGE,
	USBH_REMOVE_IMAGE,
	USBH_MAX
} usbh_M;

QueueHandle_t g_usbh_queue;
QueueHandle_t g_ceu_queue = NULL;

static void usbh_fileManager_task (void *parameters);
static int32_t read_image_file ( int file, uint8_t *buf, uint32_t file_size );
static int32_t write_image_file ( int file, uint8_t *buf, uint32_t file_size );

CMyGUI::CMyGUI(
    eC_Value x, eC_Value y,
    eC_Value width, eC_Value height,
    ObjectHandle_t eID) :
    CStreamRuntimeGUI(x, y, width, height, eID)

{

	// Get pointer to ComboBox
	m_pComboBox =  dynamic_cast<CGUIComboBox*>(GETGUI.GetObjectByID(AID_COMBOBOX_1));
	m_pDisplayMode = dynamic_cast<CGUICheckBox*>(GETGUI.GetObjectByID(AID_CHECKBOX_1));
	m_pDisplayModeLabel = static_cast<CGUITextField*>(GETGUI.GetObjectByID(AID_TEXTFIELD_1));

	// USB Host Message Queue
	g_usbh_queue = xQueueCreate(20, sizeof(usbh_M));
	// CEU Camera Control
	g_ceu_queue = xQueueCreate(20, sizeof(uint8_t));

	// Mount all Devices
	if ( 0 < dskMountAllDevices() ) {
		f_chdrive("A:\\");
		CreateFileList("*.jpg");
	}

	// Create USB Task
	R_OS_CreateTask("USBH MSC Task",usbh_fileManager_task, NULL, R_OS_ABSTRACTION_PRV_DEFAULT_STACK_SIZE, TASK_GRAPHICS_TASK_PRI+5);

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
	uint8_t ceu_switch = 1;

	if ( "DisplayMode" == kAPI ) {

		if ( m_pDisplayMode->IsSelected() ) {

			// Set Label to Image
			m_pDisplayModeLabel->SetLabel("Image");

			// Stop CEU capture
			ceu_switch = 0;
			xQueueSend ( g_ceu_queue, (void*)&ceu_switch, 0);

		} else {
			// Set Label to Video
			m_pDisplayModeLabel->SetLabel("Video");

			// Start CEU Capture
			ceu_switch = 1;
			xQueueSend ( g_ceu_queue, (void*)&ceu_switch, 0);
		}
	} else if ( "imageCapture" == kAPI ) {

		// Stop CEU Camera Capture
		ceu_switch = 0;
		xQueueSend ( g_ceu_queue, (void*)&ceu_switch, 0);

		// Send Queue Message to USB : Capture Image from Frame buffer
		usbh_M mUSBH_Mode = USBH_CAPTURE_FRAMEBUFFER;
		xQueueSend ( g_usbh_queue, (void*)&mUSBH_Mode, 0);


	} else if ( "imageDisplay" == kAPI ) {

		// Send Queue Message to USB : Display image set in Selection box
		usbh_M mUSBH_Mode = USBH_DISPLAY_IMAGE;
		xQueueSend ( g_usbh_queue, (void*)&mUSBH_Mode, 0);
	} else if ( "imageDelete" == kAPI ) {

		// Send Queue Message to USB : Remove Selected Image File
		usbh_M mUSBH_Mode = USBH_REMOVE_IMAGE;
		xQueueSend ( g_usbh_queue, (void*)&mUSBH_Mode, 0);
	}

    return true;
}


static void usbh_fileManager_task (void *parameters) {

	usbh_M mUSBH_Mode = USBH_NORMAL;
	char FileName[100];
	char SelectedFileName[100];
	char FullFileName[100];
	int pEntry;

	uint8_t ceu_switch = 1;

	static int Numitems = 0;

	eC_String selectedFileName;


	while (1) {
		xQueueReceive ( g_usbh_queue, &mUSBH_Mode, R_OS_ABSTRACTION_PRV_EV_WAIT_INFINITE);

		switch ( mUSBH_Mode) {

			case USBH_CAPTURE_FRAMEBUFFER :

				// Create file Name
				// check for existing files that begin with "image*"
				Numitems = atoi ( &FileName[5]);
				sprintf( FileName, "image%d.jpg",++Numitems);
				sprintf( FullFileName, "%s%s",ROOT_PATH, FileName );

				// Create File
				pEntry = open( FullFileName, O_CREAT, _IONBF );
				if ( 0 < pEntry )
					close ( pEntry );

				// Write to File
				pEntry = open( FullFileName, O_RDWR, _IONBF );
				if ( 0 < pEntry ) {
					// Write Frame buffer to USB file
					write_image_file( pEntry, video_buffer, (320*240*2));

					close ( pEntry );
				}

				// Update ComboBox
				GETMYGUI.FileListUpdate( FileName );

				// Start CEU Capture
				ceu_switch = 1;
				xQueueSend ( g_ceu_queue, (void*)&ceu_switch, 0);
				break;
			case USBH_DISPLAY_IMAGE :
				// Check USB is connected
				// Get File Name From Combo box

				selectedFileName = GETMYGUI.m_pComboBox->GetSelectedItemStr();
				selectedFileName.ToASCII(SelectedFileName);
				// Open File on USB Drive
				sprintf(FullFileName, "%s%s",ROOT_PATH, SelectedFileName);
				pEntry = open( FullFileName, O_RDWR, _IONBF );

				// TODO: Decode image and write to Framebuffer Pending Driver
				// Read Image to USB Drive
				read_image_file( pEntry, video_buffer, (320*240*2));
				{
					R_RVAPI_GraphChangeSurfaceVDC( VDC_CHANNEL_0,  VDC_LAYER_ID_0_RD,
					(void*) video_buffer );
				}
				close(pEntry);
				break;
			case USBH_REMOVE_IMAGE :

				selectedFileName = GETMYGUI.m_pComboBox->GetSelectedItemStr();

				selectedFileName.ToASCII(SelectedFileName);

				printf("%s\n", SelectedFileName);
				sprintf( FullFileName, "%s%s",ROOT_PATH, SelectedFileName );

				R_FAT_RemoveFile( (char*)FullFileName );
				GETMYGUI.m_pComboBox->RemoveItem( SelectedFileName, true);
				GETMYGUI.m_pComboBox->SetSelection(0);

				break;
			default :
				R_OS_TaskSleep (2);
				break;
		}

	}
}


void CMyGUI::FileListUpdate ( char* FileName ) {
	if ( NULL != FileName && '\0' != FileName ) {
		// Create Item with file entry
		CGUIListItem * pListItem = new CGUIListItem(NULL, 0, 0, eC_FromInt(80), eC_FromInt(20), FileName );
		// Modify the newly created item's label
		pListItem->GetLabel()->SetAligned(CGUIText::V_CENTERED);
		pListItem->GetLabel()->SetTextColor(0xff000000, 0xffffffff, 0xff000000, 0xffffffff);
		// Add it to the ComboBox
		m_pComboBox->AddItem(pListItem);
	}

}
void CMyGUI::CreateFileList ( const eC_Char* pattern ) {

	char full_path[] = "A:\\";


	// Get pointer to FSFAT drive
	void *pDrive = dskGetDrive(WORKING_DRV);

	if ( pDrive == NULL)
		return;

	FATENTRY fatEntry;
	FRESULT fatResult;

	DIR dir;

	fatResult = R_FAT_FindFirst( &dir, &fatEntry, full_path, pattern);

	while ( FR_OK == fatResult ) {
		// Create Item with file entry
		CGUIListItem * pListItem = new CGUIListItem(NULL, 0, 0, eC_FromInt(80), eC_FromInt(20), fatEntry.FileName );
		// Modify the newly created item's label
		pListItem->GetLabel()->SetAligned(CGUIText::V_CENTERED);
		pListItem->GetLabel()->SetTextColor(0xff000000, 0xffffffff, 0xff000000, 0xffffffff);
		// Add it to the ComboBox
		m_pComboBox->AddItem(pListItem);

		/* Get the next one */
		fatResult = R_FAT_FindNext( &dir, &fatEntry);
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
	m_pComboBox->SetSelection(0);
	// Make the comboxbox-header editable, so that users can enter a search-string
	m_pComboBox->SetHeaderEditable(true);

}

static int32_t read_image_file ( int file, uint8_t *buf, uint32_t file_size ) {
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
			printf("Bytes Read :\t%d\n", ret);
			if ( ret > 0 ) {
				bytes_done += ret;
				bytes_remaining -= ret;
				buf += ret;
			}
	}
	while((bytes_remaining > 0) && (ret >= 0));

	return bytes_done;
}

static int32_t write_image_file ( int file, uint8_t *buf, uint32_t file_size ) {
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

				ret = write(file, buf, chunk_size);
				printf("Bytes Written :\t%d\n", ret);
				if ( ret > 0 ) {
					bytes_done += ret;
					bytes_remaining -= ret;
					buf += ret;
				}
		}
		while((bytes_remaining > 0) && (ret >= 0));

		return bytes_done;
}
