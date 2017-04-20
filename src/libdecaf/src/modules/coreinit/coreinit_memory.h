#pragma once
#include "coreinit_enum.h"
#include "kernel/kernel_enum.h"

#include <common/be_val.h>
#include <common/cbool.h>
#include <libcpu/mem.h>
#include <cstdint>

namespace coreinit
{

void *
OSBlockMove(void *dst,
            const void *src,
            uint32_t size,
            BOOL flush);

void *
OSBlockSet(void *dst,
           uint8_t val,
           uint32_t size);

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

void
OSGetDataPhysAddrRange(be_val<ppcaddr_t> *start,
                       be_val<uint32_t> *size);

void
OSGetMapVirtAddrRange(be_val<ppcaddr_t> *start,
                      be_val<uint32_t> *size);

ppcaddr_t
OSAllocVirtAddr(ppcaddr_t address,
                uint32_t size,
                uint32_t alignment);

BOOL
OSFreeVirtAddr(ppcaddr_t address,
               uint32_t size);

kernel::VirtualMemoryType
OSQueryVirtAddr(ppcaddr_t virtAddress);

BOOL
OSMapMemory(ppcaddr_t virtAddress,
            ppcaddr_t physAddress,
            uint32_t size,
            kernel::MapPermission permission);

BOOL
OSUnmapMemory(ppcaddr_t virtAddress,
              uint32_t size);

} // namespace coreinit
