/**
* Created by Innopolis Acronis Team 03.03.2018
* Module Name: DLogger.c
* \brief
* \environment Kernel mode only.
*/

#include <ntddk.h>          // various NT definitions
#include <string.h>

#include "DLogger.h"

DRIVER_INITIALIZE DriverEntry;
DRIVER_UNLOAD LoggerUnload;

/**
* \brief
* \param DriverObject
* \param RegistryPath
* \return
*/
NTSTATUS
DriverEntry(
	__in PDRIVER_OBJECT DriverObject,
	__in PUNICODE_STRING	RegistryPath
)
{
	NTSTATUS ntStatus;

	UNREFERENCED_PARAMETER(RegistryPath);

	__debugbreak();

	DriverObject->DriverUnload = LoggerUnload;

	return ntStatus;
}

/**
* \brief
* \param DriverObject
*/
VOID
LoggerUnload(
	__in PDRIVER_OBJECT DriverObject
)
{
	__debugbreak();
}
