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

#define GETMYGUI    static_cast<CMyGUI&>(GETGUI)

// Application specific CGUI instance. Implemented by customer.
class CMyGUI : public NStreamRuntime::CStreamRuntimeGUI
{

public:
    CMyGUI(eC_Value x, eC_Value y, eC_Value width, eC_Value height, ObjectHandle_t eID);
    ~CMyGUI();

    virtual void OnNotification(const CGUIValue& kObservedValue, const CGUIObject* const pkUpdatedObject, const eC_UInt uiX = 0, const eC_UInt uiY = 0);

    /// Example implementation for simple Application <-> GUI communication
    eC_Bool CallApplicationAPI(const eC_String& kAPI, const eC_String& kParam);


    static void pvLedButtonCallback(CDataPoolEntry& data);

private:

    void NotifyOfDestruction(const CGUIObject* pDestructedObject);

};

#endif //#ifndef _MYGUI_SR_H_
