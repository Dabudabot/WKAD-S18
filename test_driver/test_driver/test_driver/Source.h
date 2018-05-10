#pragma once

#include <ntddk.h>
#include <ntdef.h>
#include <WinError.h>

#include "KLogger_lib.h"

#define ONE_SECOND_TIMEOUT 10000000ll

PCHAR Message[] = {
	"[klogtest 1]: curIRQL == 0\r\n",
	"[klogtest 1]: curIRQL == 1\r\n",
	"[klogtest 1]: curIRQL == 2\r\n",
	"[klogtest 1]: curIRQL == 3\r\n",
	"[klogtest 1]: curIRQL == 4\r\n",
	"[klogtest 1]: curIRQL == 5\r\n",
	"[klogtest 1]: curIRQL == 6\r\n",
	"[klogtest 1]: curIRQL == 7\r\n",
	"[klogtest 1]: curIRQL == 8\r\n",
	"[klogtest 1]: curIRQL == 9\r\n",
	"[klogtest 1]: curIRQL == 10\r\n",
	"[klogtest 1]: curIRQL == 11\r\n",
	"[klogtest 1]: curIRQL == 12\r\n",
	"[klogtest 1]: curIRQL == 13\r\n",
	"[klogtest 1]: curIRQL == 14\r\n",
	"[klogtest 1]: curIRQL == 15\r\n"
};

HANDLE ThreadHandle;
PKTHREAD pThread;

NTSTATUS DriverEntry(
	_In_ struct _DRIVER_OBJECT *DriverObject,
	_In_ PUNICODE_STRING RegistryPath
);

VOID DriverUnload(
	_In_ struct _DRIVER_OBJECT *DriverObject
);

VOID ThreadFunc(
	IN PVOID _Unused
);