/*
* Copyright (C) TES Electronic Solutions GmbH,
* All Rights Reserved.
* Contact: info@guiliani.de
*
* This file is part of the Guiliani HMI framework
* for the development of graphical user interfaces on embedded systems.
*/

#ifndef _MYGUI_SR_H_
#define _MYGUI_SR_H_

#include "GUI.h"
#include "StreamRuntimeGUI.h"
#include "GUIObjectHandleResource.h"

#include "FreeRTOS.h"
//#include "task.h"
#include "queue.h"

#define GETMYGUI    static_cast<CMyGUI&>(GETGUI)


typedef struct usbM_t {
	bool_t left_button;
	bool_t right_button;
	int xPos;
	int yPos;
}usbM_t;
// Application specific CGUI instance. Implemented by customer.
class CMyGUI : public NStreamRuntime::CStreamRuntimeGUI, public CGUIObserver, public CGUIBehaviour
{

public:
    CMyGUI(eC_Value x, eC_Value y, eC_Value width, eC_Value height, ObjectHandle_t eID);
    ~CMyGUI();

    virtual eC_Bool DoDrag(const eC_Value& vDeltaX, const eC_Value& vDeltaY, const eC_Value& vAbsX, const eC_Value& vAbsY );

    virtual void OnNotification(const CGUIValue& kObservedValue, const CGUIObject* const pkUpdatedObject, const eC_UInt uiX = 0, const eC_UInt uiY = 0);

    /// Example implementation for simple Application <-> GUI communication
    eC_Bool CallApplicationAPI(const eC_String& kAPI, const eC_String& kParam);



private:
    usbM_t usbMouseMsg;
    os_msg_queue_handle_t m_USBMouseQ;
    bool_t m_touchPressed = false;


};

#endif //#ifndef _MYGUI_SR_H_
