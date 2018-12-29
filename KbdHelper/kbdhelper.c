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
PDRIVER_OBJECT DriverObject,
PUNICODE_STRING RegistryPath
)
{
  UNREFERENCED_PARAMETER(RegistryPath);

  DriverObject->DriverUnload = DriverUnload;

	for (int i = 1; i <= IRP_MJ_MAXIMUM_FUNCTION; i++)
	{
    DriverObject->MajorFunction[i] = DispatchPass;
	}

  DriverObject->MajorFunction[IRP_MJ_READ] = DispatchRead;

  NTSTATUS status = MyAttachDevice(DriverObject);

	return status;
}

VOID
DriverUnload(
PDRIVER_OBJECT DriverObject
)
{

  PDEVICE_OBJECT deviceObject = DriverObject->DeviceObject;

  IoDetachDevice(((PDEVICE_EXTENSION) deviceObject->DeviceExtension)->LowerDevice);
  IoDeleteDevice(deviceObject);

}

NTSTATUS
DispatchPass(
PDEVICE_OBJECT DeviceObject,
PIRP Irp
)
{
	IoCopyCurrentIrpStackLocationToNext(Irp);
	return IoCallDriver(((PDEVICE_EXTENSION)DeviceObject->DeviceExtension)->LowerDevice, Irp);
}

NTSTATUS
DispatchRead(
PDEVICE_OBJECT DeviceObject,
PIRP Irp
)
{
  IoCopyCurrentIrpStackLocationToNext(Irp);
  IoSetCompletionRoutine(Irp, ReadComplete, NULL, TRUE, TRUE, TRUE);

  return IoCallDriver(((PDEVICE_EXTENSION) DeviceObject->DeviceExtension)->LowerDevice, Irp);
}


NTSTATUS
ReadComplete(
PDEVICE_OBJECT DeviceObject,
PIRP Irp,
PVOID Context
)
{
  //TODO: logic goes here
}

NTSTATUS
MyAttachDevice(
PDRIVER_OBJECT DriverObject
)
{
	UNICODE_STRING TargetDeviceName = RTL_CONSTANT_STRING(L"\\Device\\KeyboardClass0");
	PDEVICE_OBJECT myDeviceObject = NULL;
  NTSTATUS status = STATUS_SUCCESS;

  //  create device
  status = IoCreateDevice(DriverObject, sizeof(DEVICE_EXTENSION), NULL, FILE_DEVICE_MOUSE, 0, FALSE, &myDeviceObject);
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