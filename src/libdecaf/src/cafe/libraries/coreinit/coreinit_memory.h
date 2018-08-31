#pragma once
#include "coreinit_enum.h"
#include <libcpu/be2_struct.h>

namespace cafe::coreinit
{

virt_ptr<void>
OSBlockMove(virt_ptr<void> dst,
            virt_ptr<const void> src,
            uint32_t size,
            BOOL flush);

virt_ptr<void>
OSBlockSet(virt_ptr<void> dst,
           int val,
           uint32_t size);

BOOL
OSGetForegroundBucket(virt_ptr<virt_addr> addr,
                      virt_ptr<uint32_t> size);

BOOL
OSGetForegroundBucketFreeArea(virt_ptr<virt_addr> addr,
                              virt_ptr<uint32_t> size);

int
OSGetMemBound(OSMemoryType type,
              virt_ptr<virt_addr> addr,
              virt_ptr<uint32_t> size);

void
OSGetAvailPhysAddrRange(virt_ptr<phys_addr> start,
                        virt_ptr<uint32_t> size);

void
OSGetDataPhysAddrRange(virt_ptr<phys_addr> start,
                       virt_ptr<uint32_t> size);

void
OSGetMapVirtAddrRange(virt_ptr<virt_addr> start,
                      virt_ptr<uint32_t> size);

BOOL
OSGetSharedData(OSSharedDataType type,
                uint32_t unk_r4,
                virt_ptr<virt_ptr<void>> outPtr,
                virt_ptr<uint32_t> outSize);

virt_addr
OSAllocVirtAddr(virt_addr address,
                uint32_t size,
                uint32_t alignment);

BOOL
OSFreeVirtAddr(virt_addr address,
               uint32_t size);

int
OSQueryVirtAddr(virt_addr virtAddress);

BOOL
OSMapMemory(virt_addr virtAddress,
            phys_addr physAddress,
            uint32_t size,
            int permission);

BOOL
OSUnmapMemory(virt_addr virtAddress,
              uint32_t size);

phys_addr
OSEffectiveToPhysical(virt_addr address);

virt_addr
OSPhysicalToEffectiveCached(phys_addr address);

virt_addr
OSPhysicalToEffectiveUncached(phys_addr address);

virt_ptr<void>
memcpy(virt_ptr<void> dst,
       virt_ptr<const void> src,
       uint32_t size);

virt_ptr<void>
memmove(virt_ptr<void> dst,
        virt_ptr<const void> src,
        uint32_t size);

virt_ptr<void>
memset(virt_ptr<void> dst,
       int value,
       uint32_t size);

namespace internal
{

void
initialiseMemory();

} // namespace internal

} // namespace cafe::coreinit
