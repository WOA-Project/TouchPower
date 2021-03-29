/*++
	Copyright (c) Microsoft Corporation. All Rights Reserved.
	Copyright (c) LumiaWoA authors. All Rights Reserved.

	Module Name:

		power.c

	Abstract:

		Implements internal interfaces exposing functionality needed for
		controller self-tests, which can be used to run HW testing
		on a device in the manufacturing line, etc.

	Environment:

		Kernel mode

	Revision History:

--*/

#include <internal.h>
#include <initguid.h>
#include <devguid.h>
#include <private\pep.h>
#include <power.h>
#include <power.tmh>

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, TchPowerInitialize)
#endif

NTSTATUS
TchPowerControl(
	IN PTOUCH_POWER pDeviceContext,
	IN DWORD pState
)
{
	STATE_RESULT_TYPE_V2* pepResult = NULL;
	PEP_PSTATE_RESOURCE_NODE_V2* pepRequest = NULL;
	NTSTATUS status;
	SIZE_T bytesReturned;

	Trace(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"--> TchPowerControl");

	pepResult = (STATE_RESULT_TYPE_V2*)ExAllocatePoolWithTag(
		NonPagedPool,
		sizeof(STATE_RESULT_TYPE_V2),
		TOUCH_POOL_TAG);

	if (!pepResult)
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Could not allocate PState result type v2");

		return STATUS_INSUFFICIENT_RESOURCES;
	}

	pepRequest = (PEP_PSTATE_RESOURCE_NODE_V2*)ExAllocatePoolWithTag(
		NonPagedPool,
		sizeof(PEP_PSTATE_RESOURCE_NODE_V2),
		TOUCH_POOL_TAG);

	if (!pepRequest)
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Could not allocate PState resource node v2");

		ExFreePoolWithTag(pepResult, TOUCH_POOL_TAG);

		return STATUS_INSUFFICIENT_RESOURCES;
	}

	pepRequest->hdr.version = 2;
	pepRequest->hdr.ComponentIndex = 0;
	pepRequest->hdr.PStateRequestType = PEP_PSTATE_SET_REQUEST;
	pepRequest->hdr.pUserData = NULL;
	pepRequest->PStateData->PStateIndex = pState;
	pepRequest->PStateData->PStateSetIndex = 0;

	status = PoFxPowerControl(
		pDeviceContext->PepHandle,
		&GUID_POWER_CHANGE_P_STATE_V2,
		pepRequest,
		sizeof(PEP_PSTATE_RESOURCE_NODE_V2),
		pepResult,
		sizeof(STATE_RESULT_TYPE_V2),
		&bytesReturned);

	if (!NT_SUCCESS(status))
	{
		switch (status)
		{
		case STATUS_NOT_SUPPORTED:
			Trace(
				TRACE_LEVEL_ERROR,
				TRACE_INIT,
				"TchPowerControl: STATUS_NOT_SUPPORTED error");
			break;
		case STATUS_NOT_IMPLEMENTED:
			Trace(
				TRACE_LEVEL_ERROR,
				TRACE_INIT,
				"TchPowerControl: STATUS_NOT_IMPLEMENTED error");
			break;
		default:
			Trace(
				TRACE_LEVEL_ERROR,
				TRACE_INIT,
				"TchPowerControl: Unknown error");
		}

		ExFreePoolWithTag(pepResult, TOUCH_POOL_TAG);
		ExFreePoolWithTag(pepRequest, TOUCH_POOL_TAG);

		return status;
	}

	if (pepResult->hdr.status == STATUS_WAIT_3)
	{
		Trace(
			TRACE_LEVEL_INFORMATION,
			TRACE_INIT,
			"TchPowerControl: STATUS_WAIT_3");
	}
	else if (pepResult->hdr.status == STATUS_WAIT_1)
	{
		Trace(
			TRACE_LEVEL_INFORMATION,
			TRACE_INIT,
			"TchPowerControl: STATUS_WAIT_1");
	}

	ExFreePoolWithTag(pepRequest, TOUCH_POOL_TAG);
	ExFreePoolWithTag(pepResult, TOUCH_POOL_TAG);

	Trace(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"<-- TchPowerControl");

	return status;
}

NTSTATUS
TchPowerSelfManagedIoStart(
	IN PTOUCH_POWER pDeviceContext
)
{
	NTSTATUS status = STATUS_SUCCESS;
	PPO_FX_DEVICE poFxDevice = NULL;
	PPO_FX_COMPONENT_IDLE_STATE pIdleStates = NULL;

	Trace(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"--> TchPowerSelfManagedIoStart");

	poFxDevice = (PPO_FX_DEVICE)ExAllocatePoolWithTag(NonPagedPool, sizeof(PO_FX_DEVICE), TOUCH_POOL_TAG);
	if (poFxDevice)
	{
		RtlZeroMemory(poFxDevice, sizeof(PO_FX_DEVICE));
		poFxDevice->Version = PO_FX_VERSION_V1;
		poFxDevice->ComponentCount = 1;
		poFxDevice->ComponentActiveConditionCallback = NULL;
		poFxDevice->ComponentIdleConditionCallback = NULL;
		poFxDevice->ComponentIdleStateCallback = NULL;
		poFxDevice->DevicePowerRequiredCallback = NULL;
		poFxDevice->DevicePowerNotRequiredCallback = NULL;
		poFxDevice->PowerControlCallback = NULL;
		poFxDevice->DeviceContext = NULL;

		pIdleStates = (PPO_FX_COMPONENT_IDLE_STATE)ExAllocatePoolWithTag(NonPagedPool, sizeof(PO_FX_COMPONENT_IDLE_STATE), TOUCH_POOL_TAG);
		poFxDevice->Components->IdleStates = pIdleStates;
		if (pIdleStates == NULL)
		{
			Trace(
				TRACE_LEVEL_ERROR,
				TRACE_INIT,
				"Can't allocate pool for PO_FX_COMPONENT_IDLE_STATE");

			status = STATUS_INSUFFICIENT_RESOURCES;
			goto exit;
		}

		RtlZeroMemory(poFxDevice->Components->IdleStates, sizeof(PO_FX_COMPONENT_IDLE_STATE));
		poFxDevice->Components->IdleStateCount = 1;
		poFxDevice->Components->IdleStates[0].NominalPower = PO_FX_UNKNOWN_POWER;
		poFxDevice->Components->IdleStates[0].ResidencyRequirement = 0;
		poFxDevice->Components->DeepestWakeableIdleState = 0;

		status = PoFxRegisterDevice(pDeviceContext->PhysicalDevice, poFxDevice, &pDeviceContext->PepHandle);
		if (!NT_SUCCESS(status)) {
			if (status == STATUS_DEVICE_NOT_READY)
			{
				Trace(TRACE_LEVEL_ERROR, TRACE_INIT, "PoFxRegisterDevice not ready");
				goto exit;
			}

			status = STATUS_INSUFFICIENT_RESOURCES;
			Trace(TRACE_LEVEL_ERROR, TRACE_INIT, "PoFxRegisterDevice failed %!STATUS!", status);
			goto exit;
		}

		PoFxStartDevicePowerManagement(pDeviceContext->PepHandle);
		PoFxActivateComponent(pDeviceContext->PepHandle, 0, PO_FX_FLAG_BLOCKING);
	}

exit:
	if (poFxDevice)
	{
		if (pIdleStates)
			ExFreePoolWithTag(pIdleStates, TOUCH_POOL_TAG);
		ExFreePoolWithTag(poFxDevice, TOUCH_POOL_TAG);
	}

	Trace(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"<-- TchPowerSelfManagedIoStart");

	return status;
}

VOID
TchPowerOnDeviceControl(
	IN WDFQUEUE Queue,
	IN WDFREQUEST Request,
	IN size_t OutputBufferLength,
	IN size_t InputBufferLength,
	IN ULONG IoControlCode
)
/*++

Routine Description:

	This dispatch routine allows a user-mode application to issue test
	requests to the driver for execution on the chip and reporting of
	results.

Arguments:

	Queue - Framework queue object handle
	Request - Framework request object handle
	OutputBufferLength - self-explanatory
	InputBufferLength - self-explanatory
	IoControlCode - Specifies what is being requested

Return Value:

	NTSTATUS indicating success or failure

--*/
{
	PTOUCH_POWER devContext;
	NTSTATUS status = STATUS_INVALID_PARAMETER;

	UCHAR* pInputBuffer = NULL;
	UCHAR* pOutputBuffer = NULL;
	size_t dOutputLength = 0;
	size_t dInputLength = 0;

	devContext = GetDeviceContext(WdfPdoGetParent(WdfIoQueueGetDevice(Queue)));

	if (devContext == NULL)
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"TchPowerOnDeviceControl: Could not get context");

		status = STATUS_INVALID_HANDLE;

		WdfRequestComplete(
			Request,
			status);

		return;
	}

	if (InputBufferLength != 0)
	{
		status = WdfRequestRetrieveInputBuffer(
			Request,
			InputBufferLength,
			&pInputBuffer,
			&dInputLength);

		if (!NT_SUCCESS(status) || pInputBuffer == NULL)
		{
			Trace(
				TRACE_LEVEL_ERROR,
				TRACE_INIT,
				"TchPowerOnDeviceControl: Could not get input buffer");

			WdfRequestComplete(
				Request,
				status);

			return;
		}
	}

	if (OutputBufferLength != 0)
	{
		status = WdfRequestRetrieveOutputBuffer(
			Request,
			OutputBufferLength,
			&pOutputBuffer,
			&dOutputLength);

		if (!NT_SUCCESS(status) || pOutputBuffer == NULL)
		{
			Trace(
				TRACE_LEVEL_ERROR,
				TRACE_INIT,
				"TchPowerOnDeviceControl: Could not get output buffer");

			WdfRequestComplete(
				Request,
				status);

			return;
		}
	}

	switch (IoControlCode)
	{
	case IOCTL_TOUCH_POWER_RESET:
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"TchPowerOnDeviceControl: IOCTL_TOUCH_POWER_RESET");

		if (dOutputLength < sizeof(DWORD))
		{
			status = STATUS_BUFFER_TOO_SMALL;
			WdfRequestComplete(
				Request,
				status);

			return;
		}

		*(DWORD*)pOutputBuffer = 0x10000;

		WdfRequestCompleteWithInformation(
			Request,
			status,
			0);

		return;
	}
	case IOCTL_TOUCH_POWER_TOGGLE:
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"TchPowerOnDeviceControl: IOCTL_TOUCH_POWER_TOGGLE");

		if (dInputLength < sizeof(DWORD))
		{
			Trace(
				TRACE_LEVEL_ERROR,
				TRACE_INIT,
				"TchPowerOnDeviceControl: IOCTL_TOUCH_POWER_TOGGLE STATUS_BUFFER_TOO_SMALL");

			status = STATUS_BUFFER_TOO_SMALL;
			WdfRequestComplete(
				Request,
				status);

			return;
		}

		if (*pInputBuffer == 1)
		{
			status = TchPowerControl(devContext, 0);
			if (NT_SUCCESS(status))
			{
				Trace(
					TRACE_LEVEL_ERROR,
					TRACE_INIT,
					"TchPowerOnDeviceControl: IOCTL_TOUCH_POWER_TOGGLE Switched state to 1");

				devContext->State = 1;
				WdfRequestComplete(
					Request,
					status);

				return;
			}

			Trace(
				TRACE_LEVEL_ERROR,
				TRACE_INIT,
				"TchPowerOnDeviceControl: IOCTL_TOUCH_POWER_TOGGLE Failed to Switch state to 1");

			WdfRequestComplete(
				Request,
				status);

			return;
		}

		if (*pInputBuffer == 0)
		{
			status = TchPowerControl(devContext, 1);
			if (NT_SUCCESS(status))
			{
				Trace(
					TRACE_LEVEL_ERROR,
					TRACE_INIT,
					"TchPowerOnDeviceControl: IOCTL_TOUCH_POWER_TOGGLE Switched state to 0");

				devContext->State = 0;
				WdfRequestComplete(
					Request,
					status);

				return;
			}

			Trace(
				TRACE_LEVEL_ERROR,
				TRACE_INIT,
				"TchPowerOnDeviceControl: IOCTL_TOUCH_POWER_TOGGLE Failed to Switch state to 0");

			WdfRequestComplete(
				Request,
				status);

			return;
		}

		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"TchPowerOnDeviceControl: IOCTL_TOUCH_POWER_TOGGLE Unknown state");

		WdfRequestComplete(
			Request,
			status);

		return;
	}
	case IOCTL_TOUCH_POWER_STATE:
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"TchPowerOnDeviceControl: IOCTL_TOUCH_POWER_STATE");

		if (dOutputLength >= sizeof(DWORD))
		{
			*(DWORD*)pOutputBuffer = devContext->State != 0;

			Trace(
				TRACE_LEVEL_ERROR,
				TRACE_INIT,
				"TchPowerOnDeviceControl: IOCTL_TOUCH_POWER_STATE success");

			WdfRequestCompleteWithInformation(
				Request,
				status,
				0);

			return;
		}
		status = STATUS_BUFFER_TOO_SMALL;
		WdfRequestComplete(
			Request,
			status);

		return;
	}
	default:
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"TchPowerOnDeviceControl: Unknown IOCTL");

		status = STATUS_UNSUCCESSFUL;
		WdfRequestComplete(
			Request,
			status);

		return;
	}
	}
}

VOID
TchPowerOnCreate(
	IN WDFDEVICE Device,
	IN WDFREQUEST Request,
	IN WDFFILEOBJECT FileObject
)
/*++

Routine Description:

	This dispatch routine is invoked when a user-mode application is
	opening a test session. We simply reference count the number of
	creates and put the device into test mode.

Arguments:

	Device - Framework device object representing the test device
	Request - Can be used to get create information, ununsed here.
	FileObject - Unused here.

Return Value:

	None

--*/

{
	PTOUCH_POWER devContext;
	LONG testSessionCount;

	UNREFERENCED_PARAMETER(FileObject);

	devContext = GetDeviceContext(WdfPdoGetParent(Device));
	testSessionCount = InterlockedIncrement(&(devContext->TestSessionRefCnt));

	WdfRequestComplete(
		Request,
		STATUS_SUCCESS);
}

VOID
TchPowerOnClose(
	IN WDFFILEOBJECT FileObject
)
/*++

Routine Description:

	This dispatch routine is invoked when a user-mode application is
	closing a test session. We simply reference count the number of
	closes and restore the driver to normal operation if necessary.

Arguments:

	FileObject - Unused here.

Return Value:

	None

--*/

{
	PTOUCH_POWER devContext;
	LONG testSessionCount;

	devContext = GetDeviceContext(WdfPdoGetParent(WdfFileObjectGetDevice(FileObject)));

	testSessionCount = InterlockedDecrement(&(devContext->TestSessionRefCnt));
}

NTSTATUS
TchPowerInitialize(
	IN WDFDEVICE Device
)
/*++

Routine Description:

	This function creates a PDO that will be accessed directly by a user-mode
	application in order to initiate controller testing, gather results, etc.

	The test PDO is associated with the touch device's FDO and given a
	device interface so a user-mode application to find the test device.

	The function registers EvtIoDeviceControl, EvtIoRead, and EvtIoWrite
	dispatch routines which comprise the user-mode device driver interface for
	test functionality.

Arguments:

	Device - Framework device object representing the actual touch device

Return Value:

	NTSTATUS indicating success or failure

--*/
{
	NTSTATUS status;
	PTOUCH_POWER devContext;
	PWDFDEVICE_INIT deviceInit = NULL;
	WDF_FILEOBJECT_CONFIG fileConfig;
	WDFDEVICE childDevice = NULL;
	WDF_OBJECT_ATTRIBUTES objectAttributes;
	WDF_IO_QUEUE_CONFIG queueConfig;

	DECLARE_CONST_UNICODE_STRING(deviceId, L"{9AE45E76-6EF0-4ED7-85A2-97712A20786A}\\TouchPower\0");
	DECLARE_CONST_UNICODE_STRING(hardwareId, L"TOUCH_POWER");
	DECLARE_CONST_UNICODE_STRING(instanceId, L"0\0");
	DECLARE_CONST_UNICODE_STRING(instanceSecurity, L"D:P(A;;GA;;;SY)(A;;GRGWGX;;;BA)(A;;GR;;;WD)");

	devContext = GetDeviceContext(Device);

	//
	// Create a child test PDO, the touch device is the parent
	//
	deviceInit = WdfPdoInitAllocate(Device);

	if (NULL == deviceInit)
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Error allocating WDFDEVICE_INIT structure");

		status = STATUS_INVALID_DEVICE_STATE;

		goto exit;
	}

	//
	// Assign security for this interface
	//
	status = WdfDeviceInitAssignSDDLString(
		deviceInit,
		&instanceSecurity);

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Error assigning test device object security - %!STATUS!",
			status);

		goto exit;
	}

	//
	// Indicate this PDO runs in "raw mode", so the framework doesn't
	// attempt to load a function driver -- we just want an interface
	// and non-HID stack for passing test data
	//
	status = WdfPdoInitAssignRawDevice(
		deviceInit,
		&GUID_DEVCLASS_HIDCLASS);

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Error assigning test device object as raw device - %!STATUS!",
			status);

		goto exit;
	}

	//
	// Assign device, hardware, instance IDs
	//
	status = WdfPdoInitAssignDeviceID(
		deviceInit,
		&deviceId);

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Error assigning test device ID - %!STATUS!",
			status);

		goto exit;
	}

	status = WdfPdoInitAddHardwareID(
		deviceInit,
		&hardwareId);

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Error assigning test device hardware ID - %!STATUS!",
			status);

		goto exit;
	}

	status = WdfPdoInitAssignInstanceID(
		deviceInit,
		&instanceId);

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Error assigning test device instance ID - %!STATUS!",
			status);

		goto exit;
	}

	//
	// We will want to know when a test application has opened
	// or closed a handle to the test device -- during this time
	// the touch driver's normal operation is interrupted.
	//
	WDF_FILEOBJECT_CONFIG_INIT(
		&fileConfig,
		TchPowerOnCreate,
		TchPowerOnClose,
		WDF_NO_EVENT_CALLBACK);

	WdfDeviceInitSetFileObjectConfig(
		deviceInit,
		&fileConfig,
		WDF_NO_OBJECT_ATTRIBUTES);

	//
	// Create the touch test device
	//
	WDF_OBJECT_ATTRIBUTES_INIT(&objectAttributes);

	status = WdfDeviceCreate(
		&deviceInit,
		&objectAttributes,
		&childDevice);

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Error creating test device - %!STATUS!",
			status);

		goto exit;
	}

	//
	// Set up an I/O request queue so that calling application
	// may send test requests to the device.
	//
	WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(
		&queueConfig,
		WdfIoQueueDispatchParallel);

	queueConfig.EvtIoDeviceControl = TchPowerOnDeviceControl;

	status = WdfIoQueueCreate(
		childDevice,
		&queueConfig,
		WDF_NO_OBJECT_ATTRIBUTES,
		&devContext->TestQueue);

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Error creating test device request queue - %!STATUS!",
			status);

		goto exit;
	}

	//
	// Expose a device interface for a user-mode test application
	// to access this test device
	//
	status = WdfDeviceCreateDeviceInterface(
		childDevice,
		&GUID_TOUCH_POWER_INTERFACE,
		NULL);

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Error creating test device interface - %!STATUS!",
			status);

		goto exit;
	}

	//
	// This must be called in addition to WdfPdoInitAllocate to
	// associate the test PDO just created as child of the touch
	// driver FDO
	//
	status = WdfFdoAddStaticChild(Device, childDevice);

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Error adding test PDO as child of touch FDO - %!STATUS!",
			status);

		goto exit;
	}

exit:

	if (!NT_SUCCESS(status))
	{
		if (deviceInit != NULL)
		{
			WdfDeviceInitFree(deviceInit);
		}

		if (childDevice != NULL)
		{
			WdfObjectDelete(childDevice);
		}
	}

	return status;
}
