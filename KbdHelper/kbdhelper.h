/*++
	Innopolis University 2018
	Module Name:
		kbdhelper.h
	Abstract:
		This module contains the common private declarations for the mouse packet filter
		Needed to hook mouse input and modify
	Environment:
		Kernel mode only
--*/

#pragma once
#include <ntddk.h>

/**
* \brief device extension to passthrought
*/
typedef struct _DEVICE_EXTENSION
{
	PDEVICE_OBJECT LowerDevice;
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

typedef struct _KEYBOARD_INPUT_DATA
{
  USHORT UnitId;
  USHORT MakeCode;
  USHORT Flags;
  USHORT Reserved;
  ULONG  ExtraInformation;
} KEYBOARD_INPUT_DATA, *PKEYBOARD_INPUT_DATA;

/**
 * \brief driver unload logic here
 * \param driverObject driver object itself
 */
VOID DriverUnload(PDRIVER_OBJECT DriverObject);

/**
 * \brief default pass to any callbacks
 * \param deviceObject driver object itself
 * \param irp
 * \return result of function
 */
NTSTATUS DispatchPass(PDEVICE_OBJECT DeviceObject, PIRP Irp);

/**
 * \brief
 * \param deviceObject
 * \param irp
 * \param context
 * \return result of function
 */
NTSTATUS ReadComplete(PDEVICE_OBJECT DeviceObject, PIRP Irp, PVOID Context);

/**
 * \brief spetial pass for IRP_MJ_READ
 * \param deviceObject driver object itself
 * \param irp
 * \return result of function
 */
NTSTATUS DispatchRead(PDEVICE_OBJECT DeviceObject, PIRP Irp);

/**
 * \brief attachement my device to the original driver to add our logic
 * \param driverObject driver object itself
 * \return result of function
 */
NTSTATUS MyAttachDevice(PDRIVER_OBJECT DriverObject);

/**
 * \brief driver initialization entry point
 * \param driverObject driver object itself
 * \param registryPath path
 * \return result of function
 */
NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath);