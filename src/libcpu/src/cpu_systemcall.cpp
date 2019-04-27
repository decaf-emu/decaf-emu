#include "cpu.h"

#include <atomic>

namespace cpu
{

constexpr auto MaxRegisteredSystemCalls = 0xffff; // This must be `(1<<Bits)-1` due to AND below.

struct StaticSystemCallData
{
   SystemCallHandler unknownHandler =
      [](Core *core, uint32_t id) {
         return core;
      };

   std::atomic<SystemCallHandler> handlers[MaxRegisteredSystemCalls] = { nullptr };
   std::atomic_uint32_t validHandlerID = 0;
   std::atomic_uint32_t illegalHandlerID = 0;
} sSystemCall;

void
setUnknownSystemCallHandler(SystemCallHandler handler)
{
   sSystemCall.unknownHandler = handler;
}

uint32_t
registerSystemCallHandler(SystemCallHandler handler)
{
   auto id = sSystemCall.validHandlerID++;
   sSystemCall.handlers[id] = handler;
   return 0x100000 | id;
}

uint32_t
registerIllegalSystemCall()
{
   return 0x800000 | (++sSystemCall.illegalHandlerID);
}

SystemCallHandler
getSystemCallHandler(uint32_t id)
{
   if (LIKELY(id & 0x100000)) {
      return sSystemCall.handlers[id & MaxRegisteredSystemCalls];
   }

   return sSystemCall.unknownHandler;
}

} // namespace cpu
