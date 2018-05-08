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

typedef struct {
	USHORT m_unitId;
	USHORT m_flags;
	union {
		ULONG  buttons;
		struct {
			USHORT m_buttonFlags;
			USHORT m_buttonData;
		} button_data;
	} m_buttons;
	ULONG  m_rawButtons;
	LONG   m_lastX;
	LONG   m_lastY;
	ULONG  m_extraInformation;
} mouse_input_data, *pmouse_input_data;

ULONG pendingkey = 0; // COUNTER FOR NOT FINISHED IRQLs

USHORT current_point = 0;
USHORT key_combination_size = 4;
USHORT key_combination[4];

VOID DriverUnload(PDRIVER_OBJECT driverObject);

NTSTATUS DispatchPass(PDEVICE_OBJECT deviceObject, PIRP irp);

NTSTATUS ReadComplete(PDEVICE_OBJECT deviceObject, PIRP irp, PVOID context);

NTSTATUS DispatchRead(PDEVICE_OBJECT DeviceObject, PIRP Irp);

NTSTATUS MyAttachDevice(PDRIVER_OBJECT DriverObject);

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath);