#include "Source.h"

VOID ThreadFunc(
	IN PVOID _Unused
) {
	UNREFERENCED_PARAMETER(_Unused);

	DbgPrint("1 Fast path: DPC flushing first part of messages\n");
	KIRQL StartIrql = KeGetCurrentIrql();
	for (KIRQL curIrql = StartIrql; curIrql <= HIGH_LEVEL; ++curIrql) 
	{
		KIRQL _OldIrql;
		KeRaiseIrql(curIrql, &_OldIrql);
		INT LogStat = KLoggerLog(Message[curIrql]);
		DbgPrint("[klogtest 1]: curIRQL == %d, message: %s, status: %d", 
			curIrql, 
			Message[curIrql], 
			LogStat
		);
		KeLowerIrql(StartIrql);
	}

	LARGE_INTEGER DueTime;
	DueTime.QuadPart = -ONE_SECOND_TIMEOUT;	// 10^7 * 100us = 1; relative value
	LARGE_INTEGER	Interval;
	Interval.QuadPart = -2 * ONE_SECOND_TIMEOUT;

	KeDelayExecutionThread(KernelMode, FALSE, &Interval);
	DbgPrint("2 Slow path: TIMEOUT flushing all messages");

	for (KIRQL curIrql = StartIrql; curIrql <= HIGH_LEVEL; ++curIrql) {
		KIRQL _OldIrql;
		KeRaiseIrql(curIrql, &_OldIrql);
		INT LogStat = KLoggerLog(Message[curIrql]);
		DbgPrint("[klogtest 1]: curIRQL == %d, message: %s, status: %d", 
			curIrql, 
			Message[curIrql], 
			LogStat
		);

		KeLowerIrql(StartIrql);
		KeDelayExecutionThread(KernelMode, FALSE, &Interval);
	}

	KeDelayExecutionThread(KernelMode, FALSE, &Interval);
	DbgPrint("3 Combined path: DPC (1.9 msg) and TIMEOUT (1.1 msg) flushing messages");

	for (KIRQL curIrql = StartIrql; curIrql <= HIGH_LEVEL; ++curIrql) {
		KIRQL _OldIrql;
		KeRaiseIrql(curIrql, &_OldIrql);
		INT LogStat = KLoggerLog(Message[curIrql]);
		DbgPrint("[klogtest 1]: curIRQL == %d, message: %s, status: %d", 
			curIrql,
			Message[curIrql], 
			LogStat
		);

		KeLowerIrql(StartIrql);
		if (curIrql % 3 == 0)
			KeDelayExecutionThread(KernelMode, FALSE, &Interval);
	}

	PsTerminateSystemThread(ERROR_SUCCESS);
}


NTSTATUS 
DriverEntry(
	_In_ struct _DRIVER_OBJECT *DriverObject,
	_In_ PUNICODE_STRING       RegistryPath
) {
	UNREFERENCED_PARAMETER(RegistryPath);
	// __debugbreak();
	DbgPrint("[test_driver_1]: 'DriverEntry()' is executed");
	DriverObject->DriverUnload = DriverUnload;


	DbgPrint("[test_driver_1]: 'PsCreateSystemThread()' is started");
	NTSTATUS status = PsCreateSystemThread(
		&ThreadHandle,
		THREAD_ALL_ACCESS,
		NULL,
		NULL,
		NULL,
		ThreadFunc,
		NULL);
	DbgPrint("[test_driver_1]: 'PsCreateSystemThread()' is finished");

	if (NT_SUCCESS(status)) {
		DbgPrint("[test_driver_1]: 'ObReferenceObjectByHandle()' is started");
		status = ObReferenceObjectByHandle(
			ThreadHandle,
			FILE_ANY_ACCESS,
			NULL,
			KernelMode,
			(PVOID *) &(pThread),
			NULL);
		DbgPrint("[test_driver_1]: 'ObReferenceObjectByHandle()' is finished, status %d", status);

	} else {
		DbgPrint("[test_driver_1]: error exit");
		return ERROR_TOO_MANY_TCBS;
	}

	DbgPrint("[klogger_test_1]: 'DriverEntry()' finished");
	return STATUS_SUCCESS;
}

VOID 
DriverUnload(
	_In_ struct _DRIVER_OBJECT *DriverObject
) {
	UNREFERENCED_PARAMETER(DriverObject);
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
	ZwClose(ThreadHandle);

	DbgPrint("[test_driver_1]: 'DriverUnload()' is finished");
	return;
}