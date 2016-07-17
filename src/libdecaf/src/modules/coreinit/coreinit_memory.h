#pragma once
#include "common/types.h"
#include "coreinit_enum.h"
#include "common/be_val.h"

namespace coreinit
{

void *
OSBlockMove(void *dst,
            const void *src,
            ppcsize_t size,
            BOOL flush);

void *
OSBlockSet(void *dst,
           uint8_t val,
           ppcsize_t size);

BOOL
OSGetForegroundBucket(be_val<uint32_t> *addr,
                      be_val<uint32_t> *size);

BOOL
OSGetForegroundBucketFreeArea(be_val<uint32_t> *addr,
                              be_val<uint32_t> *size);

int
OSGetMemBound(OSMemoryType type,
              be_val<uint32_t> *addr,
              be_val<uint32_t> *size);

void
OSGetAvailPhysAddrRange(be_val<ppcaddr_t> *start,
                        be_val<uint32_t> *size);

ppcaddr_t
OSAllocVirtAddr(ppcaddr_t address,
                uint32_t size,
                uint32_t alignment);

BOOL
OSFreeVirtAddr(ppcaddr_t address,
               uint32_t size);

BOOL
OSMapMemory(ppcaddr_t virtAddress,
            ppcaddr_t physAddress,
            uint32_t size,
            MEMProtectMode mode);

BOOL
OSUnmapMemory(ppcaddr_t virtAddress,
              uint32_t size);

} // namespace coreinit
