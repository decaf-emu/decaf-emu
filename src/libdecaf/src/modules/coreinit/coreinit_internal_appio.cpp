#include "coreinit.h"
#include "coreinit_core.h"
#include "coreinit_internal_appio.h"
#include "coreinit_memheap.h"
#include "coreinit_messagequeue.h"
#include "coreinit_thread.h"
#include <array>

namespace coreinit
{

static const size_t AppIoMsgQueuePerThreadSize = 256;

static OSThreadEntryPointFn
sAppIoThreadEntryPoint;

static std::array<OSThread *, CoreCount>
sAppIoThread;

static std::array<OSMessageQueue *, CoreCount>
sAppIoMsgQueue;

uint32_t
AppIoThreadEntry(uint32_t core_id, void *arg2)
{
   auto &queue = sAppIoMsgQueue[core_id];
   OSMessage msg;

   while (true) {
      OSReceiveMessage(queue, &msg, OSMessageFlags::Blocking);

      switch (static_cast<internal::AppIoEventType>(msg.args[2].value()))
      {
      default:
         throw std::logic_error("App IO thread received unrecognized event type");
      }
   }

   return 0;
}

namespace internal
{

void sendMessage(OSMessage *message) {
   auto core_id = cpu::this_core::id();
   OSSendMessage(sAppIoMsgQueue[core_id], message, OSMessageFlags::Blocking);
}

void
startAppIoThreads()
{
   for (auto i = 0u; i < CoreCount; ++i) {
      auto &thread = sAppIoThread[i];
      auto stackSize = 16 * 1024;
      auto stack = reinterpret_cast<uint8_t*>(coreinit::internal::sysAlloc(stackSize, 8));
      auto name = coreinit::internal::sysStrDup(fmt::format("I/O Thread {}", i));

      OSCreateThread(thread, sAppIoThreadEntryPoint, i, nullptr,
         reinterpret_cast<be_val<uint32_t>*>(stack + stackSize), stackSize, -1,
         static_cast<OSThreadAttributes>(1 << i));
      OSSetThreadName(thread, name);
      OSResumeThread(thread);
   }
}

} // namespace internal

void
Module::initialiseAppIo()
{
   for (auto i = 0u; i < CoreCount; ++i) {
      OSMessage *messages = static_cast<OSMessage*>(internal::sysAlloc(sizeof(OSMessage) * AppIoMsgQueuePerThreadSize));
      OSInitMessageQueue(sAppIoMsgQueue[i], messages, AppIoMsgQueuePerThreadSize);
   }
}

void
Module::registerAppIoFunctions()
{
   RegisterInternalFunction(AppIoThreadEntry, sAppIoThreadEntryPoint);
   RegisterInternalData(sAppIoMsgQueue);
   RegisterInternalData(sAppIoThread);
}

} // namespace coreinit
