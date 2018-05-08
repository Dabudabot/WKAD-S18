/*++
	Innopolis University 2018
	Module Name:
		moufiltr.h
	Abstract:
		This module contains the common private declarations for the mouse packet filter
		Needed to hook mouse input and modify
	Environment:
		Kernel mode only
--*/

#pragma once
#include <ntddk.h>

/**
 * \brief Native function to get Object and work with it by name
 * \param ObjectName name of the object needed to return
 * \param Attributes OBJ_CASE_INSENSITIVE
 * \param AccessState NULL
 * \param DesiredAccess 0
 * \param ObjectType POBJECT_TYPE
 * \param ACCESSMode KernelMode
 * \param ParseContext NULL
 * \param Object pointer to object
 * \return status success in case of success
 */
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

/**
* \brief device extension to passthrought
*/
typedef struct 
{
	PDEVICE_OBJECT m_lowerDevice;
} device_extension, *pdevice_extension;

/**
* \brief mouse data stored here
*/
typedef struct 
{
	USHORT m_unitId;
	USHORT m_flags;
	union 
	{
		ULONG  buttons;
		struct 
		{
			USHORT m_buttonFlags;
			USHORT m_buttonData;
		} button_data;
	} m_buttons;
	ULONG  m_rawButtons;
	LONG   m_lastX;
	LONG   m_lastY;
	ULONG  m_extraInformation;
} mouse_input_data, *pmouse_input_data;

/**
 * \brief current pointer in keyCombination array
 */
USHORT currentPoint = 0;
/**
 * \brief size of keyCombination array
 */
USHORT keyCombinationSize = 4;
/**
 * \brief key combination array stores ordered key combination
 */
USHORT keyCombination[4] = {0,0,0,0};
/**
 * \brief is inversing activeted ot not
 */
BOOLEAN isInverse = FALSE;
/**
 * \brief counter for not finished irqls
 */
ULONG pendingkey = 0;
/**
 * \brief object needed for ObReferenceObjectByName
 */
extern POBJECT_TYPE *IoDriverObjectType;

/**
 * \brief driver unload logic here
 * \param driverObject driver object itself
 */
VOID DriverUnload(PDRIVER_OBJECT driverObject);

/**
 * \brief default pass to any callbacks
 * \param deviceObject driver object itself
 * \param irp 
 * \return result of function
 */
NTSTATUS DispatchPass(PDEVICE_OBJECT deviceObject, PIRP irp);

/**
 * \brief 
 * \param deviceObject 
 * \param irp 
 * \param context 
 * \return result of function
 */
NTSTATUS ReadComplete(PDEVICE_OBJECT deviceObject, PIRP irp, PVOID context);

/**
 * \brief spetial pass for IRP_MJ_READ
 * \param deviceObject driver object itself
 * \param irp 
 * \return result of function
 */
NTSTATUS DispatchRead(PDEVICE_OBJECT deviceObject, PIRP irp);

/**
 * \brief attachement my device to the original driver to add our logic
 * \param driverObject driver object itself
 * \return result of function
 */
NTSTATUS MyAttachDevice(PDRIVER_OBJECT driverObject);

/**
 * \brief driver initialization entry point
 * \param driverObject driver object itself
 * \param registryPath path
 * \return result of function
 */
NTSTATUS DriverEntry(PDRIVER_OBJECT driverObject, PUNICODE_STRING registryPath);