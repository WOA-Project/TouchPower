// Copyright (c) Microsoft Corporation. All Rights Reserved.  
// Copyright (c) LumiaWoA authors. All Rights Reserved.

#pragma once

#include <wdm.h>
#include <wdf.h>
#include <hidport.h>
#define RESHUB_USE_HELPER_ROUTINES
#include <reshub.h>
#include "trace.h"

//
// Memory tags
//
#define TOUCH_POOL_TAG                  (ULONG)'RwPT'

//
// Device context
//

typedef struct _TOUCH_POWER
{
    //
    // Device related
    //
    WDFDEVICE FxDevice;
    PDEVICE_OBJECT PhysicalDevice;

    //
    // Test related
    //
    WDFQUEUE TestQueue;
    volatile LONG TestSessionRefCnt;

    // 
    // Power related
    //
    POHANDLE PepHandle;
    DWORD    State;
} TOUCH_POWER, *PTOUCH_POWER;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(TOUCH_POWER, GetDeviceContext)
