/*++
	Innopolis University 2018
	Module Name:
		Source.c
	Abstract:
		This module contains implementations
	Environment:
		Kernel mode only
--*/

#include "Source.h"

VOID DriverUnload(_In_ PDRIVER_OBJECT driverObject)
{
	UNREFERENCED_PARAMETER(driverObject);
	// __debugbreak();
	DbgPrint("[library_driver]: 'DriverUnload()' is executed");
	return;
}

NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT driverObject, _In_ PUNICODE_STRING registryPath)
{
	// __debugbreak();
	DbgPrint("[library_driver]: 'DriverEntry()' is executed");
	UNREFERENCED_PARAMETER(registryPath);
	driverObject->DriverUnload = DriverUnload;
	return STATUS_SUCCESS;
}

NTSTATUS DllInitialize(_In_ PUNICODE_STRING registryPath)
{
	DbgPrint("[library_driver]: 'DllInitialize()' is started");

	const INT err = KLoggerInit(registryPath);
	if (err != ERROR_SUCCESS)
	{
		DbgPrint("[klogger_test]: DriverEntry(): 'KLoggerInit()' returned err = %d", err);
		return err;
	}

	DbgPrint("[library_driver]: 'DllInitialize()' is finished");
	return STATUS_SUCCESS;
}

NTSTATUS DllUnload(void)
{
	DbgPrint("[library_driver]: 'DllUnload()' is started");

	KLoggerDeinit();

	DbgPrint("[library_driver]: 'DllUnload()' is finished");
	return STATUS_SUCCESS;
}