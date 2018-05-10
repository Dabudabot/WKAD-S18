/*++
	Innopolis University 2018
	Module Name:
		Source.h
	Abstract:
		This module contains the common declaration of DllInit and DllUnload
		DriverEntry and Unload required but will not be used
	Environment:
		Kernel mode only

	Original projects:
		https://github.com/asnorkin/windows_kernel_logger_driver
		http://timr.probo.com/wd3/071503/KernelDlls.htm
		https://github.com/savelyev-an/LoggerDriver
--*/

#pragma once

#include <ntddk.h>
#include <winerror.h>
#include "KLogger.h"

VOID DriverUnload(_In_ PDRIVER_OBJECT driverObject);

NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT driverObject, _In_ PUNICODE_STRING registryPath);

/**
 * \brief as soon as Driver inits our library,
 * fires KLoggerInit to initialize buffer
 * \param registryPath 
 * \return 
 */
NTSTATUS DllInitialize(_In_ PUNICODE_STRING registryPath);

/**
 * \brief s soon as Driver deinits our library
 * fires KLoggerDeinit to deinit buffer
 * \return 
 */
NTSTATUS DllUnload(void);