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

NTSTATUS ReadComplete(PDEVICE_OBJECT deviceObject, PIRP irp, PVOID context)
{
	UNREFERENCED_PARAMETER(deviceObject);
	UNREFERENCED_PARAMETER(context);
	// CHAR* KeyFlag[4] = { "KeyDowm","KeyUp","E0","E1" }; // TO REMOVE
	pmouse_input_data keys = (pmouse_input_data)irp->AssociatedIrp.SystemBuffer;

	if (irp->IoStatus.Status == STATUS_SUCCESS) {
		for (int i = 0; i < irp->IoStatus.Information / sizeof(mouse_input_data); i++) {
			KdPrint(("the button state is %x  \n", keys->m_buttons.button_data.m_buttonFlags));
		}
	}

	if (irp->PendingReturned) {
		IoMarkIrpPending(irp);
	}

	pendingkey--;
	return irp->IoStatus.Status;
}

NTSTATUS DispatchRead(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	IoCopyCurrentIrpStackLocationToNext(Irp);
	// work
	IoSetCompletionRoutine(Irp, ReadComplete, NULL, TRUE, TRUE, TRUE);

	pendingkey++;

	return IoCallDriver(((pdevice_extension)DeviceObject->DeviceExtension)->m_lowerDevice, Irp);
}

NTSTATUS MyAttachDevice(PDRIVER_OBJECT DriverObject)
{
	POBJECT_TYPE *ioDriverObjectType = NULL;
	UNICODE_STRING mouseClassName = RTL_CONSTANT_STRING(L"\\Driver\\Mouclass");
	PDRIVER_OBJECT targetDriverObject = NULL;
	PDEVICE_OBJECT currentDeviceObject = NULL;
	PDEVICE_OBJECT myDeviceObject = NULL;

	NTSTATUS status = ObReferenceObjectByName(&mouseClassName, OBJ_CASE_INSENSITIVE, NULL, 0, *ioDriverObjectType, KernelMode, NULL, (PVOID*)&targetDriverObject);
	if (!NT_SUCCESS(status)) {
		KdPrint(("ObReference  is failed \r\n"));
		return status;
	}

	ObDereferenceObject(targetDriverObject);

	currentDeviceObject = targetDriverObject->DeviceObject;

	while (currentDeviceObject != NULL) {
		// TODO reWrite to solve the problem when you get an error at not the first currentDevice -> WE NEED THE LOOP FOR THE IoDeleteDevice !!!!!!!
		status = IoCreateDevice(DriverObject, sizeof(device_extension), NULL, FILE_DEVICE_MOUSE, 0, FALSE, &myDeviceObject);
		if (!NT_SUCCESS(status)) {
			// do your work
			return status;
		}

		RtlZeroMemory(myDeviceObject->DeviceExtension, sizeof(device_extension));
		status = IoAttachDeviceToDeviceStackSafe(myDeviceObject, currentDeviceObject, &((pdevice_extension)myDeviceObject->DeviceExtension)->m_lowerDevice);

		if (!NT_SUCCESS(status)) {
			// do your work
			//IoDeleteDevice(myKbdDevice);
			return status;
		}

		myDeviceObject->Flags |= DO_BUFFERED_IO;
		myDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

		currentDeviceObject = currentDeviceObject->NextDevice;
	}


	return STATUS_SUCCESS;
}

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
	UNREFERENCED_PARAMETER(RegistryPath);

	DriverObject->DriverUnload = DriverUnload;

	for (int i = 1; i <= IRP_MJ_MAXIMUM_FUNCTION; i++) {
		DriverObject->MajorFunction[i] = DispatchPass;
	}

	DriverObject->MajorFunction[IRP_MJ_READ] = DispatchRead;

	NTSTATUS status = MyAttachDevice(DriverObject);
	if (!NT_SUCCESS(status)) {
		KdPrint(("Attaching Mouse_Filter_Driver is failed \r\n"));

	}
	else {
		KdPrint(("Attacning Mouse_Filter_Driver is succeeds \r\n"));
	}
	return status;
}