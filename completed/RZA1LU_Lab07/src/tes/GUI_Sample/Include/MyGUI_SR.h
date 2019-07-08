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
#include "GUIButton.h"
#include "GUIProgressbar.h"
#include "GUITextField.h"
#include "GUIProgressbar.h"

#include "StreamRuntimeGUI.h"
#include "GUIObjectHandleResource.h"

#define GETMYGUI    static_cast<CMyGUI&>(GETGUI)

// Application specific CGUI instance. Implemented by customer.
class CMyGUI : public NStreamRuntime::CStreamRuntimeGUI, public CGUIObserver
{

public:
    CMyGUI(eC_Value x, eC_Value y, eC_Value width, eC_Value height, ObjectHandle_t eID);
    ~CMyGUI();

    virtual void OnNotification(const CGUIValue& kObservedValue, const CGUIObject* const pkUpdatedObject, const eC_UInt uiX = 0, const eC_UInt uiY = 0);

    /// Example implementation for simple Application <-> GUI communication
    eC_Bool CallApplicationAPI(const eC_String& kAPI, const eC_String& kParam);

    virtual void DoAnimate(const eC_Value &vTimes);


private:

    uint32_t adc_val = 1;
    CGUITextField *pkTextField;
    CGUIProgressBar *pkProgressBar;
    void NotifyOfDestruction(const CGUIObject* pDestructedObject);



};

#endif //#ifndef _MYGUI_SR_H_
