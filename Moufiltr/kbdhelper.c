/*++
	Innopolis University 2018
	Module Name:
		kbdhelper.c
	Abstract:
		This module contains implementations
	Environment:
		Kernel mode only
--*/

#include "kbdhelper.h"

NTSTATUS
DriverEntry(
PDRIVER_OBJECT driverObject,
PUNICODE_STRING registryPath
)
{
	UNREFERENCED_PARAMETER(registryPath);

	driverObject->DriverUnload = DriverUnload;

	for (int i = 1; i <= IRP_MJ_MAXIMUM_FUNCTION; i++)
	{
		driverObject->MajorFunction[i] = DispatchPass;
	}

	driverObject->MajorFunction[IRP_MJ_READ] = DispatchRead;

	NTSTATUS status = MyAttachDevice(driverObject);

  if (NT_SUCCESS(status))
  {
    DbgPrint("Attach\n");
  }

	return status;
}

VOID
DriverUnload(
PDRIVER_OBJECT driverObject
)
{
	LARGE_INTEGER interval = { 0 };
	PDEVICE_OBJECT deviceObject = driverObject->DeviceObject;
	interval.QuadPart = -10 * 1000 * 1000;

	while (deviceObject)
	{
		IoDetachDevice(((pdevice_extension)deviceObject->DeviceExtension)->m_lowerDevice);
		deviceObject = deviceObject->NextDevice;
	}

	while (pendingkey)
	{
		KeDelayExecutionThread(KernelMode, FALSE, &interval);
	}

	deviceObject = driverObject->DeviceObject;
	while (deviceObject)
	{
		IoDeleteDevice(deviceObject);
		deviceObject = deviceObject->NextDevice;
	}
}

NTSTATUS
DispatchPass(
PDEVICE_OBJECT deviceObject,
PIRP irp
)
{
	IoCopyCurrentIrpStackLocationToNext(irp);
	return IoCallDriver(((pdevice_extension)deviceObject->DeviceExtension)->m_lowerDevice, irp);
}

NTSTATUS
DispatchRead(
PDEVICE_OBJECT deviceObject,
PIRP irp
)
{
	IoCopyCurrentIrpStackLocationToNext(irp);
	IoSetCompletionRoutine(irp, ReadComplete, NULL, TRUE, TRUE, TRUE);

	pendingkey++;

	return IoCallDriver(((pdevice_extension)deviceObject->DeviceExtension)->m_lowerDevice, irp);
}

//TODO: logic goes here

NTSTATUS
ReadComplete(
PDEVICE_OBJECT deviceObject,
PIRP irp,
PVOID context
)
{
	UNREFERENCED_PARAMETER(deviceObject);
	UNREFERENCED_PARAMETER(context);
	pmouse_input_data keys = (pmouse_input_data)irp->AssociatedIrp.SystemBuffer;

	if (irp->IoStatus.Status == STATUS_SUCCESS)
	{
		for (int i = 0; i < irp->IoStatus.Information / sizeof(mouse_input_data); i++)
		{
			const USHORT flag = keys->m_buttons.button_data.m_buttonFlags;

			if (flag != 0 && currentPoint != keyCombinationSize)
			{
				keyCombination[currentPoint] = flag;

				if (keyCombination[0] == 1 ||
					keyCombination[1] == 4 ||
					keyCombination[2] == 8 ||
					keyCombination[3] == 2)
				{
					keyCombination[currentPoint] = 0;
					currentPoint++;

					if (currentPoint == keyCombinationSize)
					{
						KdPrint(("Inversing Y-axis\n"));
						isInverse = !isInverse;
						currentPoint = 0;
					}
				}
				else
				{
					currentPoint = 0;
				}
			}

			if (isInverse)
			{
				keys->m_lastY = 65535 - keys->m_lastY;
			}
		}
	}

	if (irp->PendingReturned)
	{
		IoMarkIrpPending(irp);
	}

	pendingkey--;
	return irp->IoStatus.Status;
}

//TODO: change to kbd

NTSTATUS
MyAttachDevice(
PDRIVER_OBJECT driverObject
)
{
	UNICODE_STRING mouseClassName = RTL_CONSTANT_STRING(L"\\Driver\\Mouclass");
	PDRIVER_OBJECT targetDriverObject = NULL;
	PDEVICE_OBJECT currentDeviceObject = NULL;
	PDEVICE_OBJECT myDeviceObject = NULL;

	NTSTATUS status = ObReferenceObjectByName(&mouseClassName,
		OBJ_CASE_INSENSITIVE, NULL, 0, *IoDriverObjectType,
		KernelMode, NULL, (PVOID*)&targetDriverObject);
	if (!NT_SUCCESS(status))
	{
		KdPrint(("ObReference  is failed \r\n"));
		return status;
	}

	ObDereferenceObject(targetDriverObject);

	currentDeviceObject = targetDriverObject->DeviceObject;

	while (currentDeviceObject != NULL)
	{
		status = IoCreateDevice(driverObject, sizeof(device_extension), NULL, FILE_DEVICE_MOUSE,
			0, FALSE, &myDeviceObject);
		if (!NT_SUCCESS(status))
		{
			return status;
		}

		RtlZeroMemory(myDeviceObject->DeviceExtension, sizeof(device_extension));
		status = IoAttachDeviceToDeviceStackSafe(myDeviceObject, currentDeviceObject,
			&((pdevice_extension)myDeviceObject->DeviceExtension)->m_lowerDevice);

		if (!NT_SUCCESS(status))
		{
			return status;
		}

		myDeviceObject->Flags |= DO_BUFFERED_IO;
		myDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

		currentDeviceObject = currentDeviceObject->NextDevice;
	}

	return STATUS_SUCCESS;
}