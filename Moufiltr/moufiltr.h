#pragma once
#include <ntddk.h>

NTSTATUS ObReferenceObjectByName(
	IN PUNICODE_STRING ObjectName,
	IN ULONG Attributes,
	IN PACCESS_STATE AccessState,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_TYPE ObjectType,
	IN KPROCESSOR_MODE ACCESSMode,
	IN PVOID ParseContext,
	OUT PVOID *Object
);


typedef struct {
	PDEVICE_OBJECT m_lowerDevice;
} device_extension, *pdevice_extension;

extern POBJECT_TYPE *ioDriverObjectType;

typedef struct {
	USHORT m_unitId;
	USHORT m_flags;
	union {
		ULONG  buttons;
		struct {
			USHORT m_buttonFlags;
			USHORT m_buttonData;
		};
	};
	ULONG  m_rawButtons;
	LONG   m_lastX;
	LONG   m_lastY;
	ULONG  m_extraInformation;
} mouse_input_data, *pmouse_input_data;

ULONG pendingkey = 0; // COUNTER FOR NOT FINISHED IRQLs

VOID DriverUnload(PDRIVER_OBJECT driverObject);

NTSTATUS DispatchPass(PDEVICE_OBJECT deviceObject, PIRP irp);

NTSTATUS ReadComplete(PDEVICE_OBJECT DeviceObject, PIRP Irp, PVOID Context)
{
	// CHAR* KeyFlag[4] = { "KeyDowm","KeyUp","E0","E1" }; // TO REMOVE
	pmouse_input_data Keys = (pmouse_input_data)Irp->AssociatedIrp.SystemBuffer;
	int structnum = Irp->IoStatus.Information / sizeof(mouse_input_data);
	int i;

	if (Irp->IoStatus.Status == STATUS_SUCCESS) {
		for (i = 0; i < structnum; i++) {
			KdPrint(("the button state is %x  \n", Keys->m_buttonFlags));
		}
	}

	if (Irp->PendingReturned) {
		IoMarkIrpPending(Irp);
	}

	pendingkey--;
	return Irp->IoStatus.Status;
}

NTSTATUS DispatchRead(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	IoCopyCurrentIrpStackLocationToNext(Irp);
	// work
	IoSetCompletionRoutine(Irp, ReadComplete, NULL, TRUE, TRUE, TRUE);

	pendingkey++;

	return IoCallDriver(((pdevice_extension)DeviceObject->DeviceExtension)->m_lowerKbdDevice, Irp);
}

NTSTATUS MyAttachDevice(PDRIVER_OBJECT DriverObject)
{
	NTSTATUS status;
	UNICODE_STRING MouseClassName = RTL_CONSTANT_STRING(L"\\Driver\\Mouclass");
	PDRIVER_OBJECT targetDriverObject = NULL;
	PDEVICE_OBJECT currentDeviceObject = NULL;
	PDEVICE_OBJECT myDeviceObject = NULL;

	status = ObReferenceObjectByName(&MouseClassName, OBJ_CASE_INSENSITIVE, NULL, 0, *ioDriverObjectType, KernelMode, NULL, (PVOID*)&targetDriverObject);
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
		status = IoAttachDeviceToDeviceStackSafe(myDeviceObject, currentDeviceObject, &((pdevice_extension)myDeviceObject->DeviceExtension)->m_lowerKbdDevice);

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
	NTSTATUS status;
	int i;
	DriverObject->DriverUnload = DriverUnload;

	for (i = 1; i <= IRP_MJ_MAXIMUM_FUNCTION; i++) {
		DriverObject->MajorFunction[i] = DispatchPass;
	}

	DriverObject->MajorFunction[IRP_MJ_READ] = DispatchRead;
	//DriverObject->MajorFunction[IRP_MJ_PNP] =  IRP_MN_REMOVE_DEVICE OR SOMETHING ELSE UGLY - !!! SEE the Video part 2 since 25.03

	status = MyAttachDevice(DriverObject);
	if (!NT_SUCCESS(status)) {
		KdPrint(("Attaching Mouse_Filter_Driver is failed \r\n"));

	}
	else {
		KdPrint(("Attacning Mouse_Filter_Driver is succeeds \r\n"));
	}
	return status;
}