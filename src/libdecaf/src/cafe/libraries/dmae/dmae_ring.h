#pragma once
#include "dmae_enum.h"
#include <libcpu/be2_struct.h>

namespace cafe::dmae
{

using DMAETimestamp = int64_t;

void
DMAEInit();

DMAETimestamp
DMAEGetLastSubmittedTimeStamp();

DMAETimestamp
DMAEGetRetiredTimeStamp();

uint32_t
DMAEGetTimeout();

void
DMAESetTimeout(uint32_t timeout);

uint64_t
DMAECopyMem(virt_ptr<void> dst,
            virt_ptr<void> src,
            uint32_t numWords,
            DMAEEndianSwapMode endian);

uint64_t
DMAEFillMem(virt_ptr<void> dst,
            uint32_t value,
            uint32_t numDwords);

BOOL
DMAEWaitDone(DMAETimestamp timestamp);

} // namespace cafe::dmae
