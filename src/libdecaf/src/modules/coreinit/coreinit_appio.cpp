#include "coreinit.h"
#include "coreinit_appio.h"
#include "coreinit_core.h"
#include "coreinit_fs.h"
#include "coreinit_fs_cmdblock.h"
#include "coreinit_fsa.h"
#include "coreinit_thread.h"
#include "coreinit_messagequeue.h"
#include "coreinit_memheap.h"
#include "ppcutils/wfunc_call.h"

namespace coreinit
{

static constexpr size_t
MessagesPerCore = 256;

struct AppIoData
{
   std::array<OSThread, CoreCount> threads;
   std::array<OSMessageQueue, CoreCount> queues;
   std::array<OSMessage, CoreCount * MessagesPerCore> messages;
};

static AppIoData *
sAppIo;

static OSThreadEntryPointFn
sAppIoEntryPoint = nullptr;

OSMessageQueue *
OSGetDefaultAppIOQueue()
{
   return &sAppIo->queues[OSGetCoreId()];
}

namespace internal
{
   
static uint32_t
appIoThreadEntry(uint32_t coreId, void *arg2)
{
   auto queue = &sAppIo->queues[coreId];
   OSInitMessageQueue(queue, &sAppIo->messages[coreId * MessagesPerCore], MessagesPerCore);

   while (true) {
      OSMessage msg;
      OSReceiveMessage(queue, &msg, OSMessageFlags::Blocking);

      auto funcType = static_cast<OSFunctionType>(msg.args[2].value());

      switch (funcType) {
      case OSFunctionType::FsaCmdAsync:
      {
         auto result = FSAGetAsyncResult(&msg);
         if (result->userCallback) {
            result->userCallback(result->error,
                                 result->command,
                                 result->request,
                                 result->response,
                                 result->userContext);
         }
         break;
      }
      case OSFunctionType::FsCmdAsync:
      {
         auto result = FSGetAsyncResult(&msg);
         if (result->asyncData.userCallback) {
            result->asyncData.userCallback(result->client,
                                           result->block,
                                           result->status,
                                           result->asyncData.userContext);
         }
         break;
      }
      case OSFunctionType::FsCmdHandler:
      {
         fsCmdBlockHandleResult(reinterpret_cast<FSCmdBlockBody *>(msg.message.get()));
         break;
      }
      default:
         decaf_abort(fmt::format("Unimplemented OSFunctionType {}", funcType));
      }
   }
}

void
startAppIoThreads()
{
   for (auto i = 0u; i < CoreCount; ++i) {
      auto thread = &sAppIo->threads[i];
      auto stackSize = 16 * 1024;
      auto stack = reinterpret_cast<uint8_t*>(coreinit::internal::sysAlloc(stackSize, 8));
      auto name = coreinit::internal::sysStrDup(fmt::format("I/O Thread {}", i));

      OSCreateThread(thread, sAppIoEntryPoint, i, nullptr,
                     reinterpret_cast<be_val<uint32_t>*>(stack + stackSize), stackSize, -1,
                     static_cast<OSThreadAttributes>(1 << i));
      OSSetThreadName(thread, name);
      OSResumeThread(thread);
   }
}

} // namespace internal

void
Module::registerAppIoFunctions()
{
   RegisterInternalFunction(internal::appIoThreadEntry, sAppIoEntryPoint);
   RegisterInternalData(sAppIo);
}

} // namespace coreinit

