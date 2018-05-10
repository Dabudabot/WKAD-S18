/*++
	Innopolis University 2018
	Module Name:
		Source.h
	Abstract:
		This module contains the common declarations for the testing driver
		Needed to use KLoggerLog to test logger driver
	Environment:
		Kernel mode only
--*/

#pragma once
#include <ntddk.h>
#include <ntdef.h>
#include <WinError.h>

#include "KLogger_lib.h"

#define ONE_SECOND_TIMEOUT 10000000ll

PCHAR message[] = {
	"[klogtest]: curIRQL == 0\r\n",
	"[klogtest]: curIRQL == 1\r\n",
	"[klogtest]: curIRQL == 2\r\n",
	"[klogtest]: curIRQL == 3\r\n",
	"[klogtest]: curIRQL == 4\r\n",
	"[klogtest]: curIRQL == 5\r\n",
	"[klogtest]: curIRQL == 6\r\n",
	"[klogtest]: curIRQL == 7\r\n",
	"[klogtest]: curIRQL == 8\r\n",
	"[klogtest]: curIRQL == 9\r\n",
	"[klogtest]: curIRQL == 10\r\n",
	"[klogtest]: curIRQL == 11\r\n",
	"[klogtest]: curIRQL == 12\r\n",
	"[klogtest]: curIRQL == 13\r\n",
	"[klogtest]: curIRQL == 14\r\n",
	"[klogtest]: curIRQL == 15\r\n"
};

HANDLE threadHandle;
PKTHREAD pThread;

/**
 * \brief standart entry point of testing driver, fire ThreadFunc
 * \param driverObject object itself
 * \param registryPath 
 * \return status of the entry
 */
NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT driverObject, _In_ PUNICODE_STRING registryPath);

/**
 * \brief unload driver here
 * \param driverObject 
 */
VOID DriverUnload(_In_ PDRIVER_OBJECT driverObject);

/**
 * \brief execution of 3 test: 
 * without any delay, with delay on every iteration, with delay on some iterations 
 * \param unused 
 */
VOID ThreadFunc(_In_ PVOID unused);