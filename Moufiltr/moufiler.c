/*--
Copyright (c) 2008  Microsoft Corporation
Module Name:
moufiltr.c
Abstract:
Environment:
Kernel mode only- Framework Version
Notes:
--*/

#include "moufiltr.h"

VOID DriverUnload(PDRIVER_OBJECT driverObject)
{
	LARGE_INTEGER interval = { 0 };
	PDEVICE_OBJECT DeviceObject = driverObject->DeviceObject;
	interval.QuadPart = -10 * 1000 * 1000;

	while (DeviceObject) {
		IoDetachDevice(((pdevice_extension)DeviceObject->DeviceExtension)->m_lowerDevice);
		DeviceObject = DeviceObject->NextDevice;
	}


	while (pendingkey) {
		KeDelayExecutionThread(KernelMode, FALSE, &interval);
	}

	DeviceObject = driverObject->DeviceObject;
	while (DeviceObject) {
		IoDeleteDevice(DeviceObject);
		DeviceObject = DeviceObject->NextDevice;
	}

	KdPrint(("Unload MouseFilterDriver \r\n"));
}

NTSTATUS DispatchPass(PDEVICE_OBJECT deviceObject, PIRP irp)
{
	IoCopyCurrentIrpStackLocationToNext(irp);
	return IoCallDriver(((pdevice_extension)deviceObject->DeviceExtension)->m_lowerDevice, irp);
}