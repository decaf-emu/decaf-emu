#include "ios_kernel.h"
#include "ios_kernel_debug.h"
#include "ios_kernel_hardware.h"
#include "ios_kernel_heap.h"
#include "ios_kernel_ipc_thread.h"
#include "ios_kernel_process.h"
#include "ios_kernel_resourcemanager.h"
#include "ios_kernel_semaphore.h"
#include "ios_kernel_scheduler.h"
#include "ios_kernel_thread.h"
#include "ios_kernel_timer.h"

#include "ios/acp/ios_acp.h"
#include "ios/auxil/ios_auxil.h"
#include "ios/bsp/ios_bsp.h"
#include "ios/crypto/ios_crypto.h"
#include "ios/fpd/ios_fpd.h"
#include "ios/fs/ios_fs.h"
#include "ios/mcp/ios_mcp.h"
#include "ios/net/ios_net.h"
#include "ios/nim/ios_nim.h"
#include "ios/nsec/ios_nsec.h"
#include "ios/pad/ios_pad.h"
#include "ios/test/ios_test.h"
#include "ios/usb/ios_usb.h"

#include "ios/ios_enum.h"
#include "ios/ios_stackobject.h"

#include "kernel/kernel_memory.h"

#include <common/log.h>
#include <functional>

namespace ios::kernel
{

using namespace std::chrono_literals;
constexpr auto RootThreadNumMessages = 1u;
constexpr auto RootThreadStackSize = 0x2000u;
constexpr auto RootThreadPriority = 126u;

struct RootThreadMessage
{
   be2_val<RootThreadCommand> command;
   be2_array<uint32_t, 13> args;
};
CHECK_SIZE(RootThreadMessage, 0x38);

struct StaticData
{
   be2_val<ThreadId> threadId;
   be2_val<TimerId> timerId;
   be2_val<MessageQueueId> messageQueueId;
   be2_array<Message, RootThreadNumMessages> messageBuffer;
   be2_array<uint8_t, RootThreadStackSize> threadStack;
   be2_struct<RootThreadMessage> rootTimerMessage;
   be2_struct<RootThreadMessage> sysprotEventMessage;
};

static phys_ptr<StaticData>
sData;

struct ProcessInfo
{
   ProcessId pid;
   ThreadEntryFn entry;
   int32_t priority;
   uint32_t stackSize;
   uint32_t memPermMask;
};

static ProcessInfo sProcessBootInfo[] = {
   { ProcessId::MCP,    mcp::processEntryPoint,       124, 0x2000,   0xC0030 },
   { ProcessId::BSP,    bsp::processEntryPoint,       125, 0x1000,  0x100000 },
   { ProcessId::CRYPTO, crypto::processEntryPoint,    123, 0x1000,   0xC0030 },
   { ProcessId::USB,    usb::processEntryPoint,       107, 0x4000,   0x38600 },
   { ProcessId::FS,     fs::processEntryPoint,         85, 0x4000,  0x1C5870 },
   { ProcessId::PAD,    pad::processEntryPoint,       117, 0x2000,    0x8180 },
   { ProcessId::NET,    net::processEntryPoint,        80, 0x4000,    0x2000 },
   { ProcessId::NIM,    nim::processEntryPoint,        50, 0x4000,         0 },
   { ProcessId::NSEC,   nsec::processEntryPoint,       50, 0x1000,         0 },
   { ProcessId::FPD,    fpd::processEntryPoint,        50, 0x4000,         0 },
   { ProcessId::ACP,    acp::processEntryPoint,        50, 0x4000,         0 },
   { ProcessId::AUXIL,  auxil::processEntryPoint,      70, 0x4000,         0 },
   { ProcessId::TEST,   test::processEntryPoint,       75, 0x2000,         0 },
};

static Error
startProcesses(bool bootOnlyBSP)
{
   for (auto &info : sProcessBootInfo) {
      if (bootOnlyBSP) {
         if (info.pid != ProcessId::BSP) {
            continue;
         }
      } else if (info.pid == ProcessId::BSP) {
         continue;
      }

      auto stackPtr = allocProcessStatic(info.pid, info.stackSize);
      auto stackTop = phys_cast<uint8_t>(stackPtr) + info.stackSize;
      auto error = IOS_CreateThread(info.entry,
                                    phys_addr { static_cast<uint32_t>(info.pid) },
                                    stackTop,
                                    info.stackSize,
                                    info.priority,
                                    ThreadFlags::AllocateIpcBufferPool | ThreadFlags::Detached);
      if (error < Error::OK) {
         gLog->warn("Error creating process thread for pid {}, error = {}", info.pid, error);
         continue;
      }

      auto threadId = static_cast<ThreadId>(error);
      auto thread = internal::getThread(threadId);
      thread->pid = info.pid;

      error = IOS_StartThread(threadId);
      if (error < Error::OK) {
         gLog->warn("Error starting process thread for pid {}, error = {}", info.pid, error);
         continue;
      }
   }

   return Error::OK;
}

static Error
initialiseRootThread()
{
   StackObject<Message> message;
   auto error = IOS_CreateMessageQueue(phys_addrof(sData->messageBuffer),
                                       static_cast<uint32_t>(sData->messageBuffer.size()));
   if (error < Error::OK) {
      gLog->error("Failed to create root thread message queue, error = {}", error);
      return error;
   }

   sData->messageQueueId = static_cast<MessageQueueId>(error);

   error = IOS_CreateTimer(5000us,
                           1000000us,
                           sData->messageQueueId,
                           makeMessage(phys_addrof(sData->rootTimerMessage)));
   if (error < Error::OK) {
      gLog->error("Failed to create root thread timer, error = {}", error);
      return error;
   }

   sData->timerId = static_cast<TimerId>(error);

   error = IOS_ReceiveMessage(sData->messageQueueId,
                              message,
                              MessageFlags::None);
   if (error < Error::OK) {
      gLog->error("Failed to receive root thread timer message, error = {}", error);
   }

   error = IOS_HandleEvent(DeviceId::SysProt,
                           sData->messageQueueId,
                           makeMessage(phys_addrof(sData->sysprotEventMessage)));
   if (error < Error::OK) {
      gLog->error("Failed to register sysprot event handler, error = {}", error);
      return error;
   }
   return IOS_ClearAndEnable(DeviceId::SysProt);
}

static Error
handleTimerEvent()
{
   return Error::OK;
}

static Error
handleSysprotEvent()
{
   return Error::OK;
}

static Error
kernelEntryPoint(phys_ptr<void> context)
{
   StackObject<Message> message;

   // Start timer thread
   internal::startTimerThread();

   // Set initial process caps
   internal::setSecurityLevel(SecurityLevel::Normal);
   internal::setClientCapability(ProcessId::KERNEL, FeatureId { 0x7FFFFFFF }, -1);
   internal::setClientCapability(ProcessId::MCP, FeatureId { 0x7FFFFFFF }, -1);
   internal::setClientCapability(ProcessId::BSP, FeatureId { 0x7FFFFFFF }, -1);

   for (auto i = +ProcessId::CRYPTO; i < NumIosProcess; ++i) {
      internal::setClientCapability(ProcessId { i }, 1, 0xF);
   }

   // Initialise shared heap
   auto sharedHeapRange = ::kernel::getPhysicalRange(::kernel::PhysicalRegion::MEM2IosSharedHeap);
   auto error = IOS_CreateHeap(sharedHeapRange.start,
                               sharedHeapRange.size);
   if (error < Error::OK) {
      gLog->error("Failed to create shared heap, error = {}", error);
      return error;
   }

   auto sharedHeapId = static_cast<HeapId>(error);
   if (sharedHeapId != 1) {
      gLog->error("Expected IOS kernel sharedHeapId to be 1, found {}", sharedHeapId);
      return Error::Invalid;
   }

   error = IOS_CreateCrossProcessHeap(0x20000);
   if (error < Error::OK) {
      gLog->error("Failed to create cross process heap, error = {}", error);
      return error;
   }

   // Start the BSP process
   error = startProcesses(true);
   if (error < Error::OK) {
      gLog->error("Failed to start BSP process, error = {}", error);
      return error;
   }

   /* TODO: Once we have the BSP process, enable this!
   IOS_SetThreadPriority(CurrentThread, 124);
   // TODO: Wait for BSP startup complete
   IOS_SetThreadPriority(CurrentThread, 126);
   */

   // Initialise root kernel thread
   error = initialiseRootThread();
   if (error < Error::OK) {
      gLog->error("Failed to initialise root thread, error = {}", error);
      return error;
   }

   // Start the rest of the processes
   error = startProcesses(false);
   if (error < Error::OK) {
      gLog->error("Failed to start remaining IOS processes, error = {}", error);
      return error;
   }

   // Start IPC thread
   internal::startIpcThread();

   while (true) {
      error = IOS_ReceiveMessage(sData->messageQueueId, message, MessageFlags::None);
      if (error) {
         return error;
      }

      auto rootThreadMessage = phys_ptr<RootThreadMessage> { phys_addr { message } };
      switch (rootThreadMessage->command) {
      case RootThreadCommand::Timer:
         error = handleTimerEvent();
         break;
      case RootThreadCommand::SysprotEvent:
         error = handleSysprotEvent();
         break;
      default:
         gLog->warn("Received unexpected message on root thread, command = {}", rootThreadMessage->command);
      }
   }
}

Error
start()
{
   // Initialise static memory
   internal::initialiseProcessStaticAllocators();

   internal::initialiseStaticHardwareData();
   internal::initialiseStaticHeapData();
   internal::initialiseStaticSchedulerData();
   internal::initialiseStaticMessageQueueData();
   internal::initialiseStaticResourceManagerData();
   internal::initialiseStaticSemaphoreData();
   internal::initialiseStaticThreadData();
   internal::initialiseStaticTimerData();

   sData = allocProcessStatic<StaticData>();
   sData->rootTimerMessage.command = RootThreadCommand::Timer;
   sData->sysprotEventMessage.command = RootThreadCommand::SysprotEvent;

   // Create root kernel thread
   auto error = IOS_CreateThread(kernelEntryPoint,
                                 nullptr,
                                 phys_addrof(sData->threadStack),
                                 static_cast<uint32_t>(sData->threadStack.size()),
                                 RootThreadPriority,
                                 ThreadFlags::Detached);
   if (error < Error::OK) {
      return error;
   }

   // Force start the root kernel thread. We cannot use IOS_StartThread
   // because it reschedules and we are not running on an IOS thread.
   internal::lockScheduler();
   auto threadId = static_cast<ThreadId>(error);
   auto thread = internal::getThread(threadId);
   thread->state = ThreadState::Ready;
   internal::queueThreadNoLock(thread);
   internal::unlockScheduler();
   return Error::OK;
}

} // namespace ios::kernel
