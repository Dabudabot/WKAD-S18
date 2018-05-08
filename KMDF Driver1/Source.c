#include <ntddk.h>

VOID DriverUnload(_In_ struct _DRIVER_OBJECT *driverObject)
{
	UNREFERENCED_PARAMETER(driverObject);
	__debugbreak();
	return;
}

NTSTATUS DriverEntry(_In_ struct _DRIVER_OBJECT *driverObject, _In_ PUNICODE_STRING registryPath)
{
	UNREFERENCED_PARAMETER(registryPath);
	__debugbreak();
	driverObject->DriverUnload = DriverUnload;
	return STATUS_SUCCESS;
}