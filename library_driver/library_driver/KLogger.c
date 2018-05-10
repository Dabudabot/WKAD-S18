#include "KLogger.h"

INT KLoggerInit(PUNICODE_STRING registryPath) 
{
	int err = ERROR_SUCCESS;

	pKLogger = (pklogger)ExAllocatePool(NonPagedPool, sizeof(klogger));
	if (pKLogger == NULL) 
	{
		err = ERROR_NOT_ENOUGH_MEMORY;
		goto err_klogger_mem;
	}

	const SIZE_T ringBufSize = GetRingBufSize(registryPath);		//non checked moment
	err = RBInit(&(pKLogger->m_pRingBuf), ringBufSize);

	if (err != ERROR_SUCCESS) 
	{
		goto err_ring_buf_init;
	}

	KeInitializeEvent(&(pKLogger->m_flushEvent), SynchronizationEvent, FALSE);
	KeInitializeEvent(&(pKLogger->m_startEvent), SynchronizationEvent, FALSE);
	KeInitializeEvent(&(pKLogger->m_stopEvent), SynchronizationEvent, FALSE);

	pKLogger->m_isFlushDispatched = 0;
	pKLogger->m_pFlushDpc = (PKDPC)ExAllocatePool(NonPagedPool, sizeof(KDPC));
	if (!pKLogger->m_pFlushDpc) 
	{
		err = ERROR_NOT_ENOUGH_MEMORY;
		goto err_dpc_mem;
	}

	KeInitializeDpc(pKLogger->m_pFlushDpc, SetWriteEvent, NULL);

	pKLogger->m_pFlushingBuf = (PCHAR)ExAllocatePool(PagedPool, FLUSH_BUF_SIZE * sizeof(CHAR));
	if (!pKLogger->m_pFlushingBuf) 
	{
		err = ERROR_NOT_ENOUGH_MEMORY;
		goto err_flush_mem;
	}

	UNICODE_STRING uniName;
	OBJECT_ATTRIBUTES objAttr;
	RtlInitUnicodeString(&uniName, LOG_FILE_NAME);
	IO_STATUS_BLOCK ioStatusBlock;

	InitializeObjectAttributes(
		&objAttr,
		(PUNICODE_STRING)&uniName,
		OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
		NULL,
		NULL
	);

	NTSTATUS status = ZwCreateFile(
		&(pKLogger->m_fileHandle),
		FILE_APPEND_DATA,
		&objAttr,
		&ioStatusBlock,
		0,
		FILE_ATTRIBUTE_NORMAL,
		FILE_SHARE_WRITE,
		FILE_OPEN_IF,
		FILE_SYNCHRONOUS_IO_ALERT,
		NULL,
		0
	);

	if (!NT_SUCCESS(status)) 
	{
		err = ERROR_CANNOT_MAKE;
		goto err_file;
	}

	status = PsCreateSystemThread(
		&(pKLogger->m_flushingThreadHandle),
		THREAD_ALL_ACCESS,
		NULL,
		NULL,
		NULL,
		FlushingThreadFunc,
		NULL);

	if (NT_SUCCESS(status)) 
	{
		status = ObReferenceObjectByHandle(
			pKLogger->m_flushingThreadHandle,
			FILE_ANY_ACCESS,
			NULL,
			KernelMode,
			(PVOID*)&(pKLogger->m_pFlushingThread),
			NULL);

	}
	else 
	{
		err = ERROR_TOO_MANY_TCBS;
		goto err_thread;
	}

	// wait while thread start
	LARGE_INTEGER timeout;
	timeout.QuadPart = -START_TIMEOUT;

	KeWaitForSingleObject(
		&(pKLogger->m_startEvent),
		Executive,
		KernelMode,
		FALSE,
		&timeout);

	return STATUS_SUCCESS;

err_thread:
	ZwClose(pKLogger->m_fileHandle);

err_file:
	ExFreePool(pKLogger->m_pFlushingBuf);

err_flush_mem:
	ExFreePool(pKLogger->m_pFlushDpc);

err_dpc_mem:
	RBDeinit(pKLogger->m_pRingBuf);

err_ring_buf_init:
	ExFreePool(pKLogger);

err_klogger_mem:
	return err;
}

VOID KLoggerDeinit() 
{
	KeFlushQueuedDpcs();

	KeSetEvent(&(pKLogger->m_stopEvent), 0, FALSE);

	KeWaitForSingleObject(
		pKLogger->m_pFlushingThread,
		Executive,
		KernelMode,
		FALSE,
		NULL);

	ObDereferenceObject(pKLogger->m_pFlushingThread);
	ZwClose(pKLogger->m_flushingThreadHandle);

	ExFreePool(pKLogger->m_pFlushingBuf);
	ZwClose(pKLogger->m_fileHandle);

	ExFreePool(pKLogger->m_pFlushDpc);

	RBDeinit(pKLogger->m_pRingBuf);
	ExFreePool(pKLogger);
}

INT KLoggerLog(PCSTR logMsg) 
{
	int err = RBWrite(pKLogger->m_pRingBuf, logMsg, StrLen(logMsg));
	int loadFactor = RBLoadFactor(pKLogger->m_pRingBuf);
	DbgPrint("Load factor: %d\n", loadFactor);

	if (((loadFactor >= FLUSH_THRESHOLD) || (err == ERROR_INSUFFICIENT_BUFFER))) 
	{
		DbgPrint("Pre Interlocked: is flush dpc queued: %d", pKLogger->m_isFlushDispatched);
		const LONG origDst = InterlockedCompareExchange(&(pKLogger->m_isFlushDispatched), 1, 0);
		DbgPrint("Post Interlocked original value: %d, is flush dpc queued: %d", origDst, pKLogger->m_isFlushDispatched);
		if (!origDst) 
		{
			DbgPrint("Dpc is queued, load factor: %d\n", loadFactor);
			KeInsertQueueDpc(pKLogger->m_pFlushDpc, NULL, NULL);
		}
	}
	return err;
}

static INT WriteToFile(HANDLE fileHandle, PVOID buf, SIZE_T length) 
{
	IO_STATUS_BLOCK ioStatusBlock;
	const NTSTATUS status = ZwWriteFile(
		fileHandle,
		NULL,
		NULL,
		NULL,
		&ioStatusBlock,
		buf,
		(ULONG)length,
		NULL,
		NULL
	);

	return status;
}

VOID FlushingThreadFunc(PVOID unused)
{
	UNREFERENCED_PARAMETER(unused);
	KeSetEvent(&(pKLogger->m_startEvent), 0, FALSE);

	PVOID handles[2];
	handles[0] = (PVOID)&(pKLogger->m_flushEvent);
	handles[1] = (PVOID)&(pKLogger->m_stopEvent);

	LARGE_INTEGER timeout;
	timeout.QuadPart = -FLUSH_TIMEOUT;

	SIZE_T length = 0;
	while (TRUE) 
	{
		const NTSTATUS status = KeWaitForMultipleObjects(
			2,
			handles,
			WaitAny,
			Executive,
			KernelMode,
			TRUE,
			&timeout,
			NULL);

		//it differs a bit

		if (status == STATUS_TIMEOUT)
		{
			DbgPrint("Flushing thread is woken by TIMEOUT\n");
		}
			

		if (status == STATUS_WAIT_0)
		{
			DbgPrint("Flushing thread is woken by FLUSH EVENT\n");
		}
				

		if (status == STATUS_TIMEOUT || status == STATUS_WAIT_0) 
		{
			length = FLUSH_BUF_SIZE;

			const int err = RBRead(pKLogger->m_pRingBuf, pKLogger->m_pFlushingBuf, &length);
			if (err == ERROR_SUCCESS) 
			{
				const NTSTATUS writeStatus = WriteToFile(pKLogger->m_fileHandle, pKLogger->m_pFlushingBuf, length);
				if (writeStatus != STATUS_SUCCESS) 
				{
					DbgPrint("Error: can't write to log file, return code %d\n", writeStatus);
				}

			}
			else 
			{
				DbgPrint("Error: can't read from ring_buffer, return code %d\n", err);
			}

		} 
		else if (status == STATUS_WAIT_1)
		{
			KeClearEvent(&pKLogger->m_stopEvent);
			PsTerminateSystemThread(ERROR_SUCCESS); // exit
		}

		if (status == STATUS_WAIT_0) 
		{
			KeClearEvent(&pKLogger->m_flushEvent);
			if (!InterlockedExchange(&(pKLogger->m_isFlushDispatched), 0))
			{
				__debugbreak();
			}
		}
	}
}

//non checked func
SIZE_T GetRingBufSize(PUNICODE_STRING registryPath) 
{  
    HANDLE regKeyHandle;
	OBJECT_ATTRIBUTES odjAttr;
    UNICODE_STRING regKeyPath;
    ULONG keyValue = FLUSH_BUF_SIZE;

	ULONG partInfoSize;
 
    InitializeObjectAttributes(&odjAttr, registryPath, 0, NULL, NULL);
 
    NTSTATUS status = ZwCreateKey(
	    &regKeyHandle, 
	    KEY_QUERY_VALUE | KEY_SET_VALUE, 
	    &odjAttr, 
	    0,  
	    NULL, 
	    REG_OPTION_NON_VOLATILE, 
	    NULL
    );

    if (!NT_SUCCESS(status)) 
	{
        DbgPrint("[library_driver]: 'ZwCreateKey()' failed");
        return FLUSH_BUF_SIZE;
    }
 
    RtlInitUnicodeString(&regKeyPath, REGISTRY_BUF_SIZE_KEY);
   
    partInfoSize = sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(partInfoSize);
	const PKEY_VALUE_PARTIAL_INFORMATION partInfo = ExAllocatePool(PagedPool, partInfoSize);
    if (!partInfo) 
	{
        DbgPrint("[library_driver]: 'ExAllocatePool()' failed");
        ZwClose(regKeyHandle);
        return FLUSH_BUF_SIZE;
    }
 
    status = ZwQueryValueKey(regKeyHandle, &regKeyPath, KeyValuePartialInformation,
        partInfo, partInfoSize, &partInfoSize);
 
    switch (status) 
	{
        case STATUS_SUCCESS:
            DbgPrint("[library_driver]: switch: STATUS_SUCCESS");
            if (partInfo->Type == REG_DWORD && partInfo->DataLength == sizeof(ULONG)) 
			{
                RtlCopyMemory(&keyValue, partInfo->Data, sizeof(keyValue));
                ZwClose(regKeyHandle);
                ExFreePool(partInfo);
                return keyValue;
            }
            // break; - not break
 
        case STATUS_OBJECT_NAME_NOT_FOUND:
            DbgPrint("[library_driver]: switch: STATUS_OBJECT_NAME_NOT_FOUND");
            status = ZwSetValueKey(regKeyHandle, &regKeyPath, 0, REG_DWORD, &keyValue, sizeof(keyValue));
            if (!NT_SUCCESS(status)) 
			{
                ZwClose(regKeyHandle);
                ExFreePool(partInfo);
                return FLUSH_BUF_SIZE;
            }
 
            break;
 
        default:
            DbgPrint("[library_driver]: switch: default");
            ZwClose(regKeyHandle);
            ExFreePool(partInfo);
            return FLUSH_BUF_SIZE;
 
            break;
           
    }
 
    ZwClose(regKeyHandle);
    ExFreePool(partInfo);
 
    return FLUSH_BUF_SIZE;
}

static SIZE_T StrLen(PCSTR str) {
	SIZE_T length = 0;
	while (*(str + length) != '\0') {
		length++;
	}

	return length;
}

VOID SetWriteEvent(PKDPC pthisDpcObject, PVOID deferredContext, PVOID systemArgument1, PVOID systemArgument2)
{
	UNREFERENCED_PARAMETER(pthisDpcObject);
	UNREFERENCED_PARAMETER(deferredContext);
	UNREFERENCED_PARAMETER(systemArgument1);
	UNREFERENCED_PARAMETER(systemArgument2);

	DbgPrint("Set Write Event\n");
	KeSetEvent(&pKLogger->m_flushEvent, 0, FALSE);
}

