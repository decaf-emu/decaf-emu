#include "coreinit.h"
#include "coreinit_appio.h"
#include "coreinit_core.h"
#include "coreinit_fsa.h"
#include "coreinit_fs_cmdblock.h"
#include "coreinit_thread.h"
#include "coreinit_messagequeue.h"
#include "cafe/cafe_stackobject.h"
#include "cafe/cafe_ppc_interface_invoke.h"
#include <libcpu/cpu.h>

namespace cafe::coreinit
{

constexpr auto MessagesPerCore = 256u;
constexpr auto AppIoThreadStackSize = 0x2000u;

struct StaticAppIoData
{
   struct PerCoreData
   {
      be2_array<char, 16> threadName;
      be2_struct<OSThread> thread;
      be2_struct<OSMessageQueue> queue;
      be2_array<OSMessage, MessagesPerCore> messages;
      be2_array<uint8_t, AppIoThreadStackSize> stack;
   };


   be2_array<PerCoreData, CoreCount> perCoreData;
};

static virt_ptr<StaticAppIoData>
sAppIoData = nullptr;

static OSThreadEntryPointFn
sAppIoThreadEntry;

virt_ptr<OSMessageQueue>
OSGetDefaultAppIOQueue()
{
   return virt_addrof(sAppIoData->perCoreData[OSGetCoreId()].queue);
}

static uint32_t
appIoThreadEntry(uint32_t coreId,
                 virt_ptr<void> arg2)
{
   StackObject<OSMessage> msg;
   auto coreData = virt_addrof(sAppIoData->perCoreData[coreId]);
   auto queue = virt_addrof(coreData->queue);
   OSInitMessageQueue(queue,
                      virt_addrof(coreData->messages),
                      coreData->messages.size());

   while (true) {
      OSReceiveMessage(queue, msg, OSMessageFlags::Blocking);

      auto funcType = static_cast<OSFunctionType>(msg->args[2].value());
      switch (funcType) {
      case OSFunctionType::FsaCmdAsync:
      {
         auto result = FSAGetAsyncResult(msg);
         if (result->userCallback) {
            cafe::invoke(cpu::this_core::state(),
                         result->userCallback,
                         result->error,
                         result->command,
                         result->request,
                         result->response,
                         result->userContext);
         }
         break;
      }
      case OSFunctionType::FsCmdAsync:
      {
         auto result = FSGetAsyncResult(msg);
         if (result->asyncData.userCallback) {
            cafe::invoke(cpu::this_core::state(),
                         result->asyncData.userCallback,
                         result->client,
                         result->block,
                         result->status,
                         result->asyncData.userContext);
         }
         break;
      }
      case OSFunctionType::FsCmdHandler:
      {
         internal::fsCmdBlockHandleResult(virt_cast<FSCmdBlockBody *>(msg->message));
         break;
      }
      default:
         decaf_abort(fmt::format("Unimplemented OSFunctionType {}", funcType));
      }
   }
}

namespace internal
{

void
initialiseAppIoThreads()
{
   for (auto i = 0u; i < sAppIoData->perCoreData.size(); ++i) {
      auto &coreData = sAppIoData->perCoreData[i];
      auto thread = virt_addrof(coreData.thread);
      auto stack = virt_addrof(coreData.stack);
      coreData.threadName = fmt::format("I/O Thread {}", i);

      OSCreateThread(thread,
                     sAppIoThreadEntry,
                     i,
                     nullptr,
                     virt_cast<uint32_t *>(stack + coreData.stack.size()),
                     coreData.stack.size(),
                     -1,
                     static_cast<OSThreadAttributes>(1 << i));
      OSSetThreadName(thread, virt_addrof(coreData.threadName));
      OSResumeThread(thread);
   }
}

} // namespace internal

void
Library::registerAppIoSymbols()
{
   RegisterFunctionExport(OSGetDefaultAppIOQueue);

   RegisterDataInternal(sAppIoData);
   RegisterFunctionInternal(appIoThreadEntry, sAppIoThreadEntry);
}

} // namespace cafe::coreinit

