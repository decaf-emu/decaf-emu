#pragma once
#include "types.h"

/*
Unimplemented Locked Cache functions:
LCDisableDMA
LCEnableDMA
LCGetAllocatableSize
LCGetDMAQueueLength
LCGetMaxSize
LCGetUnallocated
LCHardwareIsAvailable
LCIsDMAEnabled
LCLoadDMABlocks
LCStoreDMABlocks
LCWaitDMAQueue
*/

void *
LCAlloc(uint32_t size);

void
LCDealloc(void * addr);
