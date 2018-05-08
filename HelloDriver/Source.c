#include <ntddk.h>

#define DEBUG_PRINT(_x_) DbgPrint _x_

VOID DriverUnload(_In_ struct _DRIVER_OBJECT *driverObject)
{
	UNREFERENCED_PARAMETER(driverObject);
	DEBUG_PRINT(("-DRIVER UNLOAD-"));
	return;
}

NTSTATUS DriverEntry(_In_ struct _DRIVER_OBJECT *driverObject, _In_ PUNICODE_STRING registryPath)
{
	UNREFERENCED_PARAMETER(registryPath);
	DEBUG_PRINT(("-DRIVER ENTRY-"));
	driverObject->DriverUnload = DriverUnload;
	return STATUS_SUCCESS;
}