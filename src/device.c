/*++
    Copyright (c) Microsoft Corporation. All Rights Reserved.
    Copyright (c) LumiaWoA authors. All Rights Reserved.

    Module Name:

        device.c

    Abstract:

        Code for handling WDF device-specific requests

    Environment:

        Kernel mode

    Revision History:

--*/

#include <internal.h>
#include <device.h>
#include <device.tmh>

NTSTATUS
OnPrepareHardware(
    IN WDFDEVICE FxDevice,
    IN WDFCMRESLIST FxResourcesRaw,
    IN WDFCMRESLIST FxResourcesTranslated
)
/*++

  Routine Description:

    This routine is called by the PnP manager and supplies thie device instance
    with it's SPB resources (CmResourceTypeConnection) needed to find the I2C
    driver.

  Arguments:

    FxDevice - a handle to the framework device object
    FxResourcesRaw - list of translated hardware resources that
        the PnP manager has assigned to the device
    FxResourcesTranslated - list of raw hardware resources that
        the PnP manager has assigned to the device

  Return Value:

    NTSTATUS indicating sucess or failure

--*/
{
    NTSTATUS status;
    PTOUCH_POWER devContext;

    UNREFERENCED_PARAMETER(FxResourcesRaw);
    UNREFERENCED_PARAMETER(FxResourcesTranslated);

    status = STATUS_SUCCESS;

    Trace(
        TRACE_LEVEL_INFORMATION,
        TRACE_INIT,
        "--> OnPrepareHardware");

    devContext = GetDeviceContext(FxDevice);

    if (devContext == NULL)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;

        Trace(
            TRACE_LEVEL_ERROR,
            TRACE_INIT,
            "Error getting context - 0x%08lX",
            status);

        goto exit;
    }

exit:

    Trace(
        TRACE_LEVEL_INFORMATION,
        TRACE_INIT,
        "<-- OnPrepareHardware");

    return status;
}

NTSTATUS
OnReleaseHardware(
    IN WDFDEVICE FxDevice,
    IN WDFCMRESLIST FxResourcesTranslated
)
/*++

  Routine Description:

    This routine cleans up any resources provided.

  Arguments:

    FxDevice - a handle to the framework device object
    FxResourcesRaw - list of translated hardware resources that
        the PnP manager has assigned to the device
    FxResourcesTranslated - list of raw hardware resources that
        the PnP manager has assigned to the device

  Return Value:

    NTSTATUS indicating sucesss or failure

--*/
{
    NTSTATUS status;
    PTOUCH_POWER devContext;

    UNREFERENCED_PARAMETER(FxResourcesTranslated);

    status = STATUS_SUCCESS;

    Trace(
        TRACE_LEVEL_INFORMATION,
        TRACE_INIT,
        "--> OnReleaseHardware");

    devContext = GetDeviceContext(FxDevice);

    if (devContext == NULL)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;

        Trace(
            TRACE_LEVEL_ERROR,
            TRACE_INIT,
            "Error getting context - 0x%08lX",
            status);

        goto exit;
    }

exit:

    Trace(
        TRACE_LEVEL_INFORMATION,
        TRACE_INIT,
        "<-- OnReleaseHardware");

    return status;
}

