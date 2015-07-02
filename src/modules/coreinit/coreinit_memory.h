#pragma once
#include "systemtypes.h"

enum class OSMemoryType : uint32_t
{
   MEM1 = 1,
   MEM2 = 2,
   System = 1234567
};

void *
OSBlockMove(void *dst, const void *src, size_t size, BOOL flush);

void *
OSBlockSet(void *dst, uint8_t val, size_t size);

BOOL
OSGetForegroundBucket(uint32_t *addr, uint32_t *size);

BOOL
OSGetForegroundBucketFreeArea(uint32_t *addr, uint32_t *size);

int
OSGetMemBound(OSMemoryType type, uint32_t *addr, uint32_t *size);
