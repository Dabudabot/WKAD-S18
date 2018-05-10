/*++
	Innopolis University 2018
	Module Name:
		Source.ñ
	Abstract:
		This module contains implementations
	Environment:
		Kernel mode only
--*/

#include "Source.h"

NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT driverObject, _In_ PUNICODE_STRING registryPath)
{
	UNREFERENCED_PARAMETER(registryPath);
	// __debugbreak();
	DbgPrint("[test_driver_1]: 'DriverEntry()' is executed");
	driverObject->DriverUnload = DriverUnload;

	DbgPrint("[test_driver_1]: 'PsCreateSystemThread()' is started");
	NTSTATUS status = PsCreateSystemThread(
		&threadHandle,
		THREAD_ALL_ACCESS,
		NULL,
		NULL,
		NULL,
		ThreadFunc,
		NULL);
	DbgPrint("[test_driver_1]: 'PsCreateSystemThread()' is finished");

	if (NT_SUCCESS(status))
	{
		DbgPrint("[test_driver_1]: 'ObReferenceObjectByHandle()' is started");
		status = ObReferenceObjectByHandle(
			threadHandle,
			FILE_ANY_ACCESS,
			NULL,
			KernelMode,
			(PVOID *) &(pThread),
			NULL);
		DbgPrint("[test_driver_1]: 'ObReferenceObjectByHandle()' is finished, status %d", status);
	}
	else
	{
		DbgPrint("[test_driver_1]: error exit");
		return ERROR_TOO_MANY_TCBS;
	}

	DbgPrint("[klogger_test_1]: 'DriverEntry()' finished");
	return STATUS_SUCCESS;
}

VOID DriverUnload(_In_ PDRIVER_OBJECT driverObject)
{
	UNREFERENCED_PARAMETER(driverObject);

	DbgPrint("[test_driver_1]: 'DriverUnload()' is started");

	DbgPrint("[test_driver_1]: 'KeWaitForSingleObject()' is started");
	KeWaitForSingleObject(
		pThread,
		Executive,
		KernelMode,
		FALSE,
		NULL
	);

	DbgPrint("[test_driver_1]: 'KeWaitForSingleObject()' is finished");

	ObDereferenceObject(pThread);
	ZwClose(threadHandle);

	DbgPrint("[test_driver_1]: 'DriverUnload()' is finished");
	return;
}

VOID ThreadFunc(_In_ PVOID unused)
{
	UNREFERENCED_PARAMETER(unused);

	DbgPrint("---1 Fast path: DPC flushing first part of messages---");
	const KIRQL startIrql = KeGetCurrentIrql();
	for (KIRQL curIrql = startIrql; curIrql <= HIGH_LEVEL; ++curIrql) 
	{
		KIRQL oldIrql;
		KeRaiseIrql(curIrql, &oldIrql);
		const INT logStat = KLoggerLog(message[curIrql]);
		DbgPrint("[klogtest 1]: curIRQL == %d, message: %s, status: %d", 
			curIrql, 
			message[curIrql], 
			logStat
		);
		KeLowerIrql(startIrql);
	}

	LARGE_INTEGER interval;
	interval.QuadPart = -2 * ONE_SECOND_TIMEOUT;

	KeDelayExecutionThread(KernelMode, FALSE, &interval);
	DbgPrint("---2 Slow path: TIMEOUT flushing all messages---");

	for (KIRQL curIrql = startIrql; curIrql <= HIGH_LEVEL; ++curIrql) 
	{
		KIRQL oldIrql;
		KeRaiseIrql(curIrql, &oldIrql);
		const INT logStat = KLoggerLog(message[curIrql]);
		DbgPrint("[klogtest 2]: curIRQL == %d, message: %s, status: %d", 
			curIrql, 
			message[curIrql], 
			logStat
		);

		KeLowerIrql(startIrql);
		KeDelayExecutionThread(KernelMode, FALSE, &interval);
	}

	KeDelayExecutionThread(KernelMode, FALSE, &interval);
	DbgPrint("---Combined path: DPC (1.9 msg) and TIMEOUT (1.1 msg) flushing messages---");

	for (KIRQL curIrql = startIrql; curIrql <= HIGH_LEVEL; ++curIrql) 
	{
		KIRQL oldIrql;
		KeRaiseIrql(curIrql, &oldIrql);
		const INT logStat = KLoggerLog(message[curIrql]);
		DbgPrint("[klogtest 3]: curIRQL == %d, message: %s, status: %d", 
			curIrql,
			message[curIrql], 
			logStat
		);

		KeLowerIrql(startIrql);
		if (curIrql % 3 == 0)
		{
			KeDelayExecutionThread(KernelMode, FALSE, &interval);
		}
	}

	PsTerminateSystemThread(ERROR_SUCCESS);
	return;
}

