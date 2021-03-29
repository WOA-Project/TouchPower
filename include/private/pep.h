#pragma once

DEFINE_GUID(GUID_POWER_CHANGE_P_STATE_V2, 0x9942B45EL, 0x2C94, 0x41F3, 0xA1, 0x5C, 0xC1, 0xA5, 0x91, 0xC7, 4, 0x69);

typedef enum PEPPStateRequestTypeEnum
{
	PEP_PSTATE_SET_REQUEST = 0
} PEPPStateRequestType;

typedef struct PStateSetRequestStType
{
	ULONG PStateSetIndex;
	ULONG PStateIndex;    
} PStateSetRequestSt;

typedef struct _PEP_PSTATE_RESOURCE_HDR
{
	ULONG version;
	ULONG ComponentIndex;
	PEPPStateRequestType PStateRequestType;
	void* pUserData;
} PEP_PSTATE_RESOURCE_HDR;

typedef struct _PEP_PSTATE_RESOURCE_NODE_V2
{
	PEP_PSTATE_RESOURCE_HDR hdr;
	PStateSetRequestSt PStateData[1];
} PEP_PSTATE_RESOURCE_NODE_V2, * PPEP_PSTATE_RESOURCE_NODE_V2;

typedef struct _STATE_RESULT_TYPE_HDR
{
	ULONG version;
	ULONG ComponentIndex;  
	NTSTATUS status;
	ULONG PEPStatus;
	void* pUserData;
} STATE_RESULT_TYPE_HDR;

typedef struct _STATE_RESULT_TYPE_V2
{
	STATE_RESULT_TYPE_HDR hdr;
} STATE_RESULT_TYPE_V2, * PSTATE_RESULT_TYPE_V2;