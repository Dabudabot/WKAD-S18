/*++
	Innopolis University 2018
	Module Name:
		KLogger.h
	Abstract:
		This module contains the declaration of Klogger logic
		
	Environment:
		Kernel mode only
--*/

#pragma once

#include <ntddk.h>
#include <WinError.h>
#include "RingBuffer.h"

//-----defines-------

#define LOG_FILE_NAME L"\\??\\C:\\Users\\James\\Dropbox\\1\\klogger.log"
#define FLUSH_THRESHOLD 50u // in percents
#define FLUSH_BUF_SIZE (100ull * 1024ull * 1024ull)
#define REGISTRY_BUF_SIZE_KEY L"BUF_SIZE"
#define START_TIMEOUT 50000000ll
#define FLUSH_TIMEOUT 10000000ll

//----structs--------

typedef struct KLogger
{
	pringbuffer		m_pRingBuf;
	HANDLE			m_fileHandle;
	PCHAR			m_pFlushingBuf;
	HANDLE			m_flushingThreadHandle;
	PKTHREAD		m_pFlushingThread;

	KEVENT			m_flushEvent;
	KEVENT			m_startEvent;
	KEVENT			m_stopEvent;

	LONG volatile	m_isFlushDispatched;
	PKDPC			m_pFlushDpc;

} klogger, *pklogger;

//----globals--------

pklogger pKLogger;

//----methods-------

/**
 * \brief initilization of the KLogger
 * ExAllocatePool of the structure above, calculating buffer size, SynchronizationEvent, ZwCreateFile, finally creating thread
 * \param registryPath 
 * \return 
 */
INT				KLoggerInit(_In_ PUNICODE_STRING registryPath);

/**
 * \brief deinitialization, flushing and closing
 */
VOID			KLoggerDeinit(void);

/**
 * \brief logging itself, asks ring buffer to write checks InterlockedCompareExchange
 * \param logMsg message to write
 * \return 
 */
INT				KLoggerLog(_In_ PCSTR logMsg);

/**
 * \brief write event for dpc, just flushes data to file
 * \param pthisDpcObject unused
 * \param deferredContext unused
 * \param systemArgument1 unused
 * \param systemArgument2 unused
 */
VOID			SetWriteEvent(_In_ PKDPC pthisDpcObject, _In_ PVOID deferredContext, 
							_In_ PVOID systemArgument1, _In_ PVOID systemArgument2);
/**
 * \brief writes to file by calling ZwWriteFile
 * \param fileHandle handler to the file
 * \param buf buffer to write
 * \param length size of the buffer
 * \return result
 */
static INT		WriteToFile(_In_ HANDLE fileHandle, _In_ PVOID buf, _In_ SIZE_T length);

/**
 * \brief flushing by event or by timeout
 * \param unused 
 */
VOID			FlushingThreadFunc(_In_ PVOID unused);

/**
 * \brief calculates buffer size
 * \param registryPath 
 * \return buffer size
 */
SIZE_T			GetRingBufSize(_In_ PUNICODE_STRING registryPath);

/**
 * \brief to calc lenght of the string
 * \param str input string
 * \return lenght of the string
 */
static SIZE_T	StrLen(_In_ PCSTR str);
