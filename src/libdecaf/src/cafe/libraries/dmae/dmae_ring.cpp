#include "dmae.h"
#include "dmae_ring.h"
#include "cafe/libraries/coreinit/coreinit_mutex.h"
#include "cafe/libraries/coreinit/coreinit_time.h"
#include "cafe/libraries/tcl/tcl_ring.h"
#include "cafe/cafe_stackobject.h"
#include <cstring>

namespace cafe::dmae
{

using namespace cafe::tcl;

struct StaticRingData
{
   be2_struct<coreinit::OSMutex> mutex;
   be2_val<uint32_t> timeout;
   be2_val<DMAETimestamp> lastSubmittedTimestamp;
};

static virt_ptr<StaticRingData>
sRingData = nullptr;

void
DMAEInit()
{
   coreinit::OSInitMutex(virt_addrof(sRingData->mutex));
}

DMAETimestamp
DMAEGetLastSubmittedTimeStamp()
{
   return sRingData->lastSubmittedTimestamp;
}

DMAETimestamp
DMAEGetRetiredTimeStamp()
{
   StackObject<TCLTimestamp> timestamp;
   TCLReadTimestamp(TCLTimestampID::DMAERetired, timestamp);
   return *timestamp;
}

uint32_t
DMAEGetTimeout()
{
   return sRingData->timeout;
}

void
DMAESetTimeout(uint32_t timeout)
{
   sRingData->timeout = timeout;
}

uint64_t
DMAECopyMem(virt_ptr<void> dst,
            virt_ptr<void> src,
            uint32_t numWords,
            DMAEEndianSwapMode endian)
{
   coreinit::OSLockMutex(virt_addrof(sRingData->mutex));

   if (endian == DMAEEndianSwapMode::None) {
      std::memcpy(dst.getRawPointer(),
                  src.getRawPointer(),
                  numWords * 4);
   } else if (endian == DMAEEndianSwapMode::Swap8In16) {
      auto dstWords = reinterpret_cast<uint16_t *>(dst.getRawPointer());
      auto srcWords = reinterpret_cast<uint16_t *>(src.getRawPointer());
      for (auto i = 0u; i < numWords * 2; ++i) {
         *dstWords++ = byte_swap(*srcWords++);
      }
   } else if (endian == DMAEEndianSwapMode::Swap8In32) {
      auto dstDwords = reinterpret_cast<uint32_t *>(dst.getRawPointer());
      auto srcDwords = reinterpret_cast<uint32_t *>(src.getRawPointer());
      for (auto i = 0u; i < numWords; ++i) {
         *dstDwords++ = byte_swap(*srcDwords++);
      }
   }

   auto timestamp = coreinit::OSGetTime();
   sRingData->lastSubmittedTimestamp = timestamp;

   coreinit::OSUnlockMutex(virt_addrof(sRingData->mutex));
   return timestamp;
}

uint64_t
DMAEFillMem(virt_ptr<void> dst,
            uint32_t value,
            uint32_t numDwords)
{
   coreinit::OSLockMutex(virt_addrof(sRingData->mutex));

   auto dstValue = byte_swap(value);
   auto dstDwords = reinterpret_cast<uint32_t *>(dst.getRawPointer());
   for (auto i = 0u; i < numDwords; ++i) {
      dstDwords[i] = dstValue;
   }

   auto timestamp = coreinit::OSGetTime();
   sRingData->lastSubmittedTimestamp = timestamp;

   coreinit::OSUnlockMutex(virt_addrof(sRingData->mutex));
   return timestamp;
}

BOOL
DMAEWaitDone(DMAETimestamp timestamp)
{
   return TRUE;
}

void
Library::registerRingSymbols()
{
   RegisterFunctionExport(DMAEInit);
   RegisterFunctionExport(DMAEGetLastSubmittedTimeStamp);
   RegisterFunctionExport(DMAEGetRetiredTimeStamp);
   RegisterFunctionExport(DMAEGetTimeout);
   RegisterFunctionExport(DMAESetTimeout);
   RegisterFunctionExport(DMAECopyMem);
   RegisterFunctionExport(DMAEFillMem);
   RegisterFunctionExport(DMAEWaitDone);
}

} // namespace cafe::dmae
