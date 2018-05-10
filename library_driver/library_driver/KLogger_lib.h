/*++
	Innopolis University 2018
	Module Name:
		KLogger_lib.h
	Abstract:
		Export functions defined here

	Environment:
		Kernel mode only
--*/

#pragma once
#include <ntdef.h>

DECLSPEC_IMPORT INT KLoggerLog(PCSTR log_msg);