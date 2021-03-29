/*++
    Copyright (c) Microsoft Corporation. All Rights Reserved. 
    Copyright (c) LumiaWoA authors. All Rights Reserved.

    Module Name:

        driver.h

    Abstract:

        Contains WDF driver-specific function declarations

    Environment:

        Kernel mode

    Revision History:

--*/

#pragma once


DRIVER_INITIALIZE DriverEntry;

EVT_WDF_DEVICE_CONTEXT_CLEANUP OnContextCleanup;

EVT_WDF_DRIVER_DEVICE_ADD OnDeviceAdd;

EVT_WDF_DEVICE_SELF_MANAGED_IO_INIT OnDeviceSelfManagedIoStart;
