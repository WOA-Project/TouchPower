/*++
    Copyright (c) Microsoft Corporation. All Rights Reserved. 
    Copyright (c) LumiaWoA authors. All Rights Reserved.

    Module Name:

        device.h

    Abstract:

        Declarations of WDF device specific entry points

    Environment:

        Kernel mode

    Revision History:

--*/

#pragma once

EVT_WDF_DEVICE_PREPARE_HARDWARE OnPrepareHardware;

EVT_WDF_DEVICE_RELEASE_HARDWARE OnReleaseHardware;