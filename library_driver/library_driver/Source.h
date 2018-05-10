#pragma once

#include <ntddk.h>
#include <winerror.h>
#include "KLogger.h"

VOID DriverUnload(
	_In_ struct _DRIVER_OBJECT *DriverObject
);

NTSTATUS DriverEntry(
	_In_ struct _DRIVER_OBJECT *DriverObject,
	_In_ PUNICODE_STRING       RegistryPath
);

NTSTATUS DllInitialize(
	_In_ PUNICODE_STRING RegistryPath
);

NTSTATUS DllUnload(void);