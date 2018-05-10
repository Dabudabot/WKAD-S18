/*++
	Innopolis University 2018
	Module Name:
		RingBuffer.c
	Abstract:
		This module contains implementations
	Environment:
		Kernel mode only
--*/

#include "RingBuffer.h"

INT RBInit(pringbuffer* pRingBuf, SIZE_T size) 
{
	INT err = ERROR_SUCCESS;

	if (!pRingBuf) 
	{
		err = ERROR_BAD_ARGUMENTS;
		goto err_ret;
	}

	pringbuffer ringBuf = (pringbuffer)ExAllocatePool(NonPagedPool, sizeof(ringbuffer));
	if (!ringBuf) 
	{
		err = ERROR_NOT_ENOUGH_MEMORY;
		goto err_ret;
	}

	*pRingBuf = ringBuf;

	ringBuf->m_data = (PCHAR)ExAllocatePool(NonPagedPool, size * sizeof(CHAR));
	if (!ringBuf->m_data) 
	{
		err = ERROR_NOT_ENOUGH_MEMORY;
		ExFreePool(ringBuf);
		goto err_ret;
	}

	ringBuf->m_head = ringBuf->m_data;
	ringBuf->m_tail = ringBuf->m_data;
	ringBuf->m_capacity = size;

	KeInitializeSpinLock(&(ringBuf->m_splockReadWrite));

err_ret:
	return err;
}

INT RBDeinit(pringbuffer pRingBuf) 
{
	if (!pRingBuf) 
	{
		return ERROR_BAD_ARGUMENTS;
	}

	ExFreePool(pRingBuf->m_data);
	ExFreePool(pRingBuf);

	return ERROR_SUCCESS;
}

SIZE_T RBSize(PCHAR head, PCHAR tail, SIZE_T capacity) 
{
	if (head >= tail) 
	{
		return (SIZE_T)(head - tail);
	} 
	return (SIZE_T)(capacity - (tail - head));
}

static SIZE_T RBFreeSize(PCHAR head, PCHAR tail, SIZE_T capacity) 
{
	return capacity - RBSize(head, tail, capacity);
}

static INT RingDataWrite(PCHAR srcBuf, SIZE_T srcBufSize, PCHAR data,
							SIZE_T capacity, PCHAR head, PCHAR tail, PCHAR* newHead) 
{
	if (head >= tail) 
	{
		SIZE_T distToFinish = capacity - (head - data);
		if (srcBufSize > distToFinish) 
		{
			RtlCopyMemory(head, srcBuf, distToFinish);
			RtlCopyMemory(data, srcBuf + distToFinish, srcBufSize - distToFinish);
			*newHead = data + srcBufSize - distToFinish;

		} else 
		{
			RtlCopyMemory(head, srcBuf, srcBufSize);
			*newHead = head + srcBufSize;
		}

	} 
	else 
	{
		RtlCopyMemory(head, srcBuf, srcBufSize);
		*newHead = head + srcBufSize;
	}

	return ERROR_SUCCESS;
}

INT RBWrite(pringbuffer pRingBuf, PCHAR pBuf, SIZE_T size) 
{
	if (!pRingBuf) 
	{
		return ERROR_BAD_ARGUMENTS;
	}

	KIRQL OldIrql;
	KeRaiseIrql(HIGH_LEVEL, &OldIrql);
	KeAcquireSpinLockAtDpcLevel(&(pRingBuf->m_splockReadWrite));

	PCHAR Head = pRingBuf->m_head;
	PCHAR Tail = pRingBuf->m_tail;

	int Err;
	if (size > RBFreeSize(Head, Tail, pRingBuf->m_capacity)) 
	{
		Err = ERROR_INSUFFICIENT_BUFFER;
		goto out;
	}

	PCHAR NewHead;
	Err = RingDataWrite(
			pBuf, 
			size, 
			pRingBuf->m_data, 
			pRingBuf->m_capacity,
			Head,
			Tail, 
			&NewHead
	);
	if (Err != ERROR_SUCCESS) 
	{
		goto out;
	}

	pRingBuf->m_head = NewHead;

out:
	KeReleaseSpinLockFromDpcLevel(&(pRingBuf->m_splockReadWrite));
	KeLowerIrql(OldIrql);

	return Err;
}

static INT RingDataRead(PCHAR pDstBuf, SIZE_T dstBufSize, PCHAR data, SIZE_T capacity, 
				PCHAR head, PCHAR tail, PSIZE_T pRetSize, PCHAR* newTail) 
{
	SIZE_T size = RBSize(head, tail, capacity);
	SIZE_T retSize = (dstBufSize < size) ? dstBufSize : size;
	*pRetSize = retSize;

	if (head >= tail) 
	{
		RtlCopyMemory(pDstBuf, tail, retSize);
		*newTail = tail + retSize;

	} 
	else
	{
		SIZE_T DistToFlush = capacity - (tail - data);
		if (retSize <= DistToFlush) 
		{
			RtlCopyMemory(pDstBuf, tail, retSize);
			*newTail = tail + retSize;

		} 
		else 
		{
			RtlCopyMemory(pDstBuf, tail, DistToFlush);
			RtlCopyMemory(pDstBuf + DistToFlush, data, retSize - DistToFlush);
			*newTail = data + retSize - DistToFlush;
		}
	}

	return ERROR_SUCCESS;
}

INT RBRead(pringbuffer pRingBuf, PCHAR pBuf, PSIZE_T pSize)
{
	if (!pRingBuf || !pSize) 
	{
		return ERROR_BAD_ARGUMENTS;
	}

	PCHAR Head = pRingBuf->m_head;
	PCHAR Tail = pRingBuf->m_tail;

	SIZE_T RetSize;
	PCHAR NewTail;
	int Err = RingDataRead(
				pBuf, 
				*pSize, 
				pRingBuf->m_data, 
				pRingBuf->m_capacity, 
				Head, 
				Tail, 
				&RetSize, 
				&NewTail
	);
	if (Err != ERROR_SUCCESS) {
		goto out;
	}

	*pSize = RetSize;
	pRingBuf->m_tail = NewTail;

out:
	return Err;
}

INT RBLoadFactor(pringbuffer pRingBuf) 
{
	return (INT)(100 * RBSize(pRingBuf->m_head, pRingBuf->m_tail, pRingBuf->m_capacity)) / pRingBuf->m_capacity;
}
