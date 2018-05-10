#pragma once

#include <ntddk.h>
#include <WinError.h>
#include "RingBuffer.h"

#define LOG_FILE_NAME L"\\??\\C:\\Users\\James\\Dropbox\\1\\klogger.log"
#define FLUSH_THRESHOLD 50u // in percents
#define DEFAULT_RING_BUF_SIZE (100ull * 1024ull * 1024ull)
#define FLUSH_BUF_SIZE DEFAULT_RING_BUF_SIZE
#define REGISTRY_BUF_SIZE_KEY L"BUF_SIZE"
#define FLUSH_TIMEOUT 10000000ll
#define START_TIMEOUT 50000000ll

typedef struct KLogger
{
	PRINGBUFFER pRingBuf;

	HANDLE FileHandle;
	PCHAR pFlushingBuf;

	HANDLE FlushingThreadHandle;
	PKTHREAD pFlushingThread;

	KEVENT FlushEvent;
	KEVENT StartEvent;
	KEVENT StopEvent;

	LONG volatile IsFlushDispatched;
	PKDPC pFlushDpc;

} KLOGGER, *PKLOGGER;

PKLOGGER gKLogger;
//typedef struct KLogger* PKLOGGER;

INT KLoggerInit(PUNICODE_STRING RegistryPath);
VOID KLoggerDeinit();
INT KLoggerLog(PCSTR log_msg);

VOID SetWriteEvent(
	IN PKDPC pthisDpcObject,
	IN PVOID DeferredContext,
	IN PVOID SystemArgument1,
	IN PVOID SystemArgument2
);

static INT
WriteToFile(
	HANDLE FileHandle,
	PVOID Buf,
	SIZE_T Length
);

VOID
FlushingThreadFunc(
	IN PVOID _Unused
);

SIZE_T GetRingBufSize(
	PUNICODE_STRING RegistryPath
);

static SIZE_T
StrLen(
	PCSTR Str
);
