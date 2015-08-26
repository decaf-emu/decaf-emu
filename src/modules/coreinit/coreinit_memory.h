#pragma once
#include "systemtypes.h"

enum class OSMemoryType : uint32_t
{
   MEM1 = 1,
   MEM2 = 2
};

void *
OSBlockMove(void *dst, const void *src, ppcsize_t size, BOOL flush);

void *
OSBlockSet(void *dst, uint8_t val, ppcsize_t size);

BOOL
OSGetForegroundBucket(be_val<uint32_t> *addr, be_val<uint32_t> *size);

BOOL
OSGetForegroundBucketFreeArea(be_val<uint32_t> *addr, be_val<uint32_t> *size);

int
OSGetMemBound(OSMemoryType type, be_val<uint32_t> *addr, be_val<uint32_t> *size);

int
OSSetMemBound(OSMemoryType type, uint32_t start, uint32_t size);
