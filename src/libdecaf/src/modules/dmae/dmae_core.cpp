#include "dmae.h"
#include "modules/coreinit/coreinit_time.h"

namespace dmae
{

enum class DMAEEndianSwapMode : uint32_t
{
   None = 0,
   Swap8In16 = 1,
   Swap8In32 = 2
};

static uint32_t
sDmaeTimeout = 5;

static coreinit::OSTime
sLastTimeStamp = 0;

coreinit::OSTime
DMAEGetRetiredTimeStamp()
{
   return sLastTimeStamp;
}

coreinit::OSTime
DMAEGetLastSubmittedTimeStamp()
{
   return sLastTimeStamp;
}

BOOL
DMAEWaitDone(coreinit::OSTime timestamp)
{
   return TRUE;
}

coreinit::OSTime
DMAECopyMem(void *dst, void *src, uint32_t numDwords, DMAEEndianSwapMode endian)
{
   if (endian == DMAEEndianSwapMode::None) {
      memcpy(dst, src, numDwords * 4);
   } else if (endian == DMAEEndianSwapMode::Swap8In16) {
      auto dstWords = reinterpret_cast<uint16_t*>(dst);
      auto srcWords = reinterpret_cast<uint16_t*>(src);
      for (uint32_t i = 0; i < numDwords * 2; ++i) {
         *dstWords++ = byte_swap(*srcWords++);
      }
   } else if (endian == DMAEEndianSwapMode::Swap8In32) {
      auto dstDwords = reinterpret_cast<uint32_t*>(dst);
      auto srcDwords = reinterpret_cast<uint32_t*>(src);
      for (uint32_t i = 0; i < numDwords; ++i) {
         *dstDwords++ = byte_swap(*srcDwords++);
      }
   }

   sLastTimeStamp = coreinit::OSGetTime();
   return sLastTimeStamp;
}

coreinit::OSTime
DMAEFillMem(void *dst, uint32_t value, uint32_t numDwords)
{
   // Note that rather than using be_val here, we simply byte-swap
   //  the incoming word before we enter the loop to write it out.
   auto dstValue = byte_swap(value);
   auto dstDwords = reinterpret_cast<uint32_t*>(dst);
   for (uint32_t i = 0; i < numDwords; ++i) {
      dstDwords[i] = dstValue;
   }

   sLastTimeStamp = coreinit::OSGetTime();
   return sLastTimeStamp;
}

void
DMAESetTimeout(uint32_t timeout)
{
   sDmaeTimeout = timeout;
}

uint32_t
DMAEGetTimeout()
{
   return sDmaeTimeout;
}

void
Module::registerCoreFunctions()
{
   RegisterKernelFunction(DMAEGetRetiredTimeStamp);
   RegisterKernelFunction(DMAEGetLastSubmittedTimeStamp);
   RegisterKernelFunction(DMAEWaitDone);
   RegisterKernelFunction(DMAECopyMem);
   RegisterKernelFunction(DMAEFillMem);
   RegisterKernelFunction(DMAESetTimeout);
   RegisterKernelFunction(DMAEGetTimeout);
}

} // namespace sysapp
