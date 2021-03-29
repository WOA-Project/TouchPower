/*++
    Copyright (c) Microsoft Corporation. All Rights Reserved.
    Copyright (c) LumiaWoA authors. All Rights Reserved.

    Module Name:

        driver.c

    Abstract:

        Contains driver-specific WDF functions

    Environment:

        Kernel mode

    Revision History:

--*/

#include <internal.h>
#include <driver.h>
#include <device.h>
#include <power.h>
#include <driver.h>
#include <driver.tmh>

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, OnContextCleanup)
#endif

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
)
/*++

Routine Description:

    Installable driver initialization entry point.
    This entry point is called directly by the I/O system.

Arguments:

    DriverObject - pointer to the driver object
    RegistryPath - pointer to a unicode string representing the path,
                   to driver-specific key in the registry.

Return Value:

    STATUS_SUCCESS if successful, error code otherwise.

--*/
{
    WDF_OBJECT_ATTRIBUTES attributes;
    WDF_DRIVER_CONFIG config;
    NTSTATUS status;

    //
    // Initialize tracing via WPP
    //
    WPP_INIT_TRACING(DriverObject, RegistryPath);

    //
    // Create a framework driver object
    //
    WDF_DRIVER_CONFIG_INIT(&config, OnDeviceAdd);
    config.DriverPoolTag = TOUCH_POOL_TAG;

    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.EvtCleanupCallback = OnContextCleanup;

    status = WdfDriverCreate(
        DriverObject,
        RegistryPath,
        &attributes,
        &config,
        WDF_NO_HANDLE
    );

    if (!NT_SUCCESS(status))
    {
        Trace(
            TRACE_LEVEL_ERROR,
            TRACE_INIT,
            "Error creating WDF driver object - 0x%08lX",
            status);

        WPP_CLEANUP(DriverObject);

        goto exit;
    }

exit:

    return status;
}

NTSTATUS
OnDeviceSelfManagedIoStart(
    WDFDEVICE Device
)
{
    NTSTATUS status;
    PTOUCH_POWER devContext;

    devContext = GetDeviceContext(Device);

    status = TchPowerSelfManagedIoStart(devContext);

    if (!NT_SUCCESS(status))
    {
        Trace(
            TRACE_LEVEL_ERROR,
            TRACE_INIT,
            "TchPowerSelfManagedIoStart failed - 0x%08lX",
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
OnDeviceAdd(
    IN WDFDRIVER Driver,
    IN PWDFDEVICE_INIT DeviceInit
)
/*++

Routine Description:

    OnDeviceAdd is called by the framework in response to AddDevice
    call from the PnP manager when a device is found. We create and
    initialize a WDF device object to represent the new instance of
    an touch device. Per-device objects are also instantiated.

Arguments:

    Driver - Handle to a framework driver object created in DriverEntry
    DeviceInit - Pointer to a framework-allocated WDFDEVICE_INIT structure.

Return Value:

    NTSTATUS indicating success or failure

--*/
{
    WDF_OBJECT_ATTRIBUTES attributes;
    PTOUCH_POWER devContext;
    WDFDEVICE fxDevice;
    WDF_PNPPOWER_EVENT_CALLBACKS pnpPowerCallbacks;
    NTSTATUS status;

    UNREFERENCED_PARAMETER(Driver);
    PAGED_CODE();

    //
    // Get power policy ownership
    //
    WdfDeviceInitSetPowerPolicyOwnership(DeviceInit, TRUE);

    //
    // This driver handles D0 Entry and Exit to power on and off the
    // controller. It also handles prepare/release hardware so that it 
    // can acquire and free SPB resources needed to communicate with the  
    // touch controller.
    //
    WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnpPowerCallbacks);

    pnpPowerCallbacks.EvtDevicePrepareHardware = OnPrepareHardware;
    pnpPowerCallbacks.EvtDeviceReleaseHardware = OnReleaseHardware;
    pnpPowerCallbacks.EvtDeviceSelfManagedIoInit = OnDeviceSelfManagedIoStart;

    WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit, &pnpPowerCallbacks);

    //
    // Create a framework device object. This call will in turn create
    // a WDM device object, attach to the lower stack, and set the
    // appropriate flags and attributes.
    //
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, TOUCH_POWER);

    status = WdfDeviceCreate(&DeviceInit, &attributes, &fxDevice);

    if (!NT_SUCCESS(status))
    {
        Trace(
            TRACE_LEVEL_ERROR,
            TRACE_INIT,
            "WdfDeviceCreate failed - 0x%08lX",
            status);

        goto exit;
    }

    devContext = GetDeviceContext(fxDevice);
    devContext->FxDevice = fxDevice;
    devContext->PhysicalDevice = WdfDeviceWdmGetPhysicalDevice(fxDevice);

    //
    // Initialize driver path for self-test
    //
    status = TchPowerInitialize(fxDevice);

    if (!NT_SUCCESS(status))
    {
        Trace(
            TRACE_LEVEL_ERROR,
            TRACE_INIT,
            "Error initializing self-test functionality - %!STATUS!",
            status);

        goto exit;
    }

exit:

    return status;
}

VOID
OnContextCleanup(
    IN WDFOBJECT Driver
    )
/*++
Routine Description:

    Free resources allocated in DriverEntry that are not automatically
    cleaned up framework.

Arguments:

    Driver - handle to a WDF Driver object.

Return Value:

    VOID.

--*/
{
    PAGED_CODE();

    WPP_CLEANUP(WdfDriverWdmGetDriverObject(Driver));
}