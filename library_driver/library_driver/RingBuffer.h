/*++
	Innopolis University 2018
	Module Name:
		RingBuffer.h
	Abstract:
		This module contains the declaration of RingBuffer logic
	Environment:
		Kernel mode only
--*/

#pragma once

#include "ntddk.h"
#include <winerror.h>

/**
 * \brief Ring buffer structure
 */
typedef struct RingBuffer {
	PCHAR m_data;
	PCHAR volatile m_head;
	PCHAR volatile m_tail;

	ULONGLONG m_capacity;

	KSPIN_LOCK m_splockReadWrite;

} ringbuffer, *pringbuffer;

/**
 * \brief initializes ring buffer structure by allocating (ExAllocatePool) for structure and for data 
 * \param pRingBuf ring buffer structure object pointer
 * \param size size of buffer
 * \return status
 */
INT		RBInit(_In_ pringbuffer* pRingBuf, _In_ SIZE_T size);

/**
 * \brief deinitialize ring buffer structure by freeing the pool
 * \param pRingBuf ring buffer sturcture object pointer
 * \return status
 */
INT		RBDeinit(_In_ pringbuffer pRingBuf);

/**
 * \brief rises irql and call RingDataWrite assign new head and restores irql lvl
 * \param pRingBuf buffer itself
 * \param pBuf what to write
 * \param size size of data to write
 * \return result
 */
INT		RBWrite(_In_ pringbuffer pRingBuf, _In_  PCHAR pBuf, _In_ SIZE_T size);

/**
 * \brief writes to buffer
 * \param srcBuf write this
 * \param srcBufSize size of write data
 * \param data 
 * \param capacity buffer size
 * \param head pointer to head of buffer
 * \param tail pointer to tail of buffer
 * \param newHead updated head
 * \return result
 */
static INT RingDataWrite(_In_ PCHAR srcBuf, _In_ SIZE_T srcBufSize, _In_ PCHAR data,
	_In_ SIZE_T capacity, _In_ PCHAR head, _In_ PCHAR tail, _Out_ PCHAR* newHead);

/**
 * \brief calls RingDataRead and assign new tail
 * \param pRingBuf buffer itself
 * \param pBuf where to read
 * \param pSize size of data to read
 * \return result
 */
INT		RBRead(_In_ pringbuffer pRingBuf, _In_ PCHAR pBuf, _In_ PSIZE_T pSize);

/**
 * \brief reads from buffer
 * \param pDstBuf where to read
 * \param dstBufSize size of read
 * \param data 
 * \param capacity buffer size
 * \param head pointer to head of buffer
 * \param tail pointer to tail of buffer
 * \param pRetSize readed size
 * \param newTail updated tail
 * \return result
 */
static INT RingDataRead(_Out_ PCHAR pDstBuf, _Out_ SIZE_T dstBufSize, _In_  PCHAR data, _In_  SIZE_T capacity,
	_In_ PCHAR head, _In_  PCHAR tail, _In_  PSIZE_T pRetSize, _Out_  PCHAR* newTail);

/**
 * \brief to get buffer size
 * \param head of buffer
 * \param tail of buffer
 * \param capacity total size of buffer
 * \return size of buffer
 */
SIZE_T	RBSize(_In_ PCHAR head, _In_ PCHAR tail, _In_ SIZE_T capacity);

/**
 * \brief how much of buffer is buzy
 * \param pRingBuf ring buffer sturcture object pointer
 * \return result
 */
INT		RBLoadFactor(_In_ pringbuffer pRingBuf);