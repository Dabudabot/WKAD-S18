#pragma once

#include "ntddk.h"
#include <winerror.h>

typedef struct RingBuffer {
	PCHAR Data;
	PCHAR volatile Head;
	PCHAR volatile Tail;

	ULONGLONG Capacity;

	KSPIN_LOCK SplockReadWrite;

} RINGBUFFER, *PRINGBUFFER;

INT RBInit(PRINGBUFFER* pRingBuf, SIZE_T Size);
INT RBDeinit(PRINGBUFFER pRingBuf);
INT RBWrite(PRINGBUFFER pRingBuf, PCHAR pBuf, SIZE_T Size);
INT RBRead(PRINGBUFFER pRingBuf, PCHAR pBuf, PSIZE_T pSize);
SIZE_T RBSize(PCHAR Head, PCHAR Tail, SIZE_T Capacity);
INT RBLoadFactor(PRINGBUFFER pRingBuf);