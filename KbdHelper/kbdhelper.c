/*++

  Filter keyboard driver

  0. Start in "start state"
  1. Listen to input and wait for "init sequence" to enter "active state"
  2. Listen to input and wait for "cmd sequence" to enter "record state"
  3. Record sequence, step time and capture time goto "active state"
  4. As soon as capture time + step time < current time append sequence,
      update capture time
  5. As soon as input "exit sequence", clean memory and goto "start state"

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
  //  TODO: change

  PDEVICE_OBJECT deviceObject = driverObject->DeviceObject;

  IoDetachDevice(((PDEVICE_EXTENSION) deviceObject->DeviceExtension)->LowerDevice);
  IoDeleteDevice(deviceObject);

	LARGE_INTEGER interval = { 0 };

	interval.QuadPart = -10 * 1000 * 1000;

	while (deviceObject)
	{
		IoDetachDevice(((PDEVICE_EXTENSION)deviceObject->DeviceExtension)->LowerDevice);
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
	return IoCallDriver(((PDEVICE_EXTENSION)deviceObject->DeviceExtension)->LowerDevice, irp);
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

  return IoCallDriver(((PDEVICE_EXTENSION) deviceObject->DeviceExtension)->LowerDevice, irp);
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
	UNICODE_STRING TargetDeviceName = RTL_CONSTANT_STRING(L"\\Device\\KeyboardClass0");
	PDEVICE_OBJECT myDeviceObject = NULL;
  NTSTATUS status = STATUS_SUCCESS;

  //  create device
	status = IoCreateDevice(driverObject, sizeof(DEVICE_EXTENSION), NULL, FILE_DEVICE_MOUSE, 0, FALSE, &myDeviceObject);
	if (!NT_SUCCESS(status))
	{
		return status;
	}

  //  set flags
  myDeviceObject->Flags |= DO_BUFFERED_IO;
  myDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

  //  zero extension
  RtlZeroMemory(myDeviceObject->DeviceExtension, sizeof(DEVICE_EXTENSION));

  //  attach to stack
  status = IoAttachDevice(myDeviceObject, &TargetDeviceName, &((PDEVICE_EXTENSION) myDeviceObject->DeviceExtension)->LowerDevice);

	if (!NT_SUCCESS(status))
	{
    IoDeleteDevice(myDeviceObject);
		return status;
	}

	return status;
}