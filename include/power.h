/*++
    Copyright (c) Microsoft Corporation. All Rights Reserved.
    Copyright (c) LumiaWoA authors. All Rights Reserved.

    Module Name:

        power.h

    Abstract:

        Contains internal interfaces exposing functionality needed for 
        controller self-tests, which can be used to run HW testing 
        on a device in the manufacturing line, etc.

    Environment:

        Kernel mode

    Revision History:

--*/

#pragma once


//
// This GUID is used to access the touch self-test virtual device from user-mode
//
DEFINE_GUID(GUID_TOUCH_POWER_INTERFACE,
   0x9AE45E76, 0x6EF0, 0x4ED7, 0x85, 0xA2, 0x97, 0x71, 0x2A, 0x20, 0x78, 0x6A);
// {9AE45E76-6EF0-4ED7-85A2-97712A20786A}

#define TOUCH_TEST_BUFFER_CTL_CODE(id)  \
    CTL_CODE(0x8323, (id), METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_TOUCH_POWER_RESET           TOUCH_TEST_BUFFER_CTL_CODE(0x801)
#define IOCTL_TOUCH_POWER_TOGGLE          TOUCH_TEST_BUFFER_CTL_CODE(0x802)
#define IOCTL_TOUCH_POWER_STATE           TOUCH_TEST_BUFFER_CTL_CODE(0x803)

EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL TchPowerOnDeviceControl;

EVT_WDF_DEVICE_FILE_CREATE TchPowerOnCreate;

EVT_WDF_FILE_CLOSE TchPowerOnClose;

NTSTATUS
TchPowerInitialize(
    IN WDFDEVICE Device
);

NTSTATUS
TchPowerSelfManagedIoStart(
    IN PTOUCH_POWER Context
);