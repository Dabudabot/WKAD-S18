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
	//__debugbreak();
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
	//__debugbreak();
	IoCopyCurrentIrpStackLocationToNext(irp);
	return IoCallDriver(((pdevice_extension)deviceObject->DeviceExtension)->m_lowerDevice, irp);
}

NTSTATUS ReadComplete(PDEVICE_OBJECT deviceObject, PIRP irp, PVOID context)
{
	//__debugbreak();
	UNREFERENCED_PARAMETER(deviceObject);
	UNREFERENCED_PARAMETER(context);
	// CHAR* KeyFlag[4] = { "KeyDowm","KeyUp","E0","E1" }; // TO REMOVE
	pmouse_input_data keys = (pmouse_input_data)irp->AssociatedIrp.SystemBuffer;

	if (irp->IoStatus.Status == STATUS_SUCCESS) {
		for (int i = 0; i < irp->IoStatus.Information / sizeof(mouse_input_data); i++) {

			KdPrint(("m_lastX: %x m_lastY: %x\n", keys->m_lastX, keys->m_lastY));

			const USHORT flag = keys->m_buttons.button_data.m_buttonFlags;
			if (flag != 0)
			{
				KdPrint(("the button state is %x pointer is %d \n", flag, current_point));

				key_combination[current_point] = flag;

				if (key_combination[0] == 1 || key_combination[1] == 4 || key_combination[2] == 8 || key_combination[3] == 2)
				{
					key_combination[current_point] = 0;
					current_point++;
				} 
				else
				{
					current_point = 0;
				}

				if (current_point == key_combination_size)
				{
					KdPrint(("------INVERSE ACTIVATED------\n"));
					current_point = 0;

					const LONG temp = keys->m_lastX;
					keys->m_lastX = keys->m_lastY;
					keys->m_lastY = temp;
				}
			}
			
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
	//__debugbreak();
	IoCopyCurrentIrpStackLocationToNext(Irp);
	// work
	IoSetCompletionRoutine(Irp, ReadComplete, NULL, TRUE, TRUE, TRUE);

	pendingkey++;

	return IoCallDriver(((pdevice_extension)DeviceObject->DeviceExtension)->m_lowerDevice, Irp);
}

extern POBJECT_TYPE *IoDriverObjectType;

NTSTATUS MyAttachDevice(PDRIVER_OBJECT DriverObject)
{
	//__debugbreak();
	UNICODE_STRING mouseClassName = RTL_CONSTANT_STRING(L"\\Driver\\Mouclass");
	PDRIVER_OBJECT targetDriverObject = NULL;
	PDEVICE_OBJECT currentDeviceObject = NULL;
	PDEVICE_OBJECT myDeviceObject = NULL;

	NTSTATUS status = ObReferenceObjectByName(&mouseClassName, OBJ_CASE_INSENSITIVE, NULL, 0, *IoDriverObjectType, KernelMode, NULL, (PVOID*)&targetDriverObject);
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
	for (int i = 0; i < key_combination_size; i++)
	{
		key_combination[i] = 0;
	}
	//__debugbreak();
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