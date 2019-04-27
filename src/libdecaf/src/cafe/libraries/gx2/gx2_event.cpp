#include "gx2.h"
#include "gx2_cbpool.h"
#include "gx2_event.h"
#include "gx2_state.h"

#include "cafe/cafe_ppc_interface_invoke.h"
#include "cafe/cafe_stackobject.h"
#include "cafe/libraries/coreinit/coreinit_alarm.h"
#include "cafe/libraries/coreinit/coreinit_memheap.h"
#include "cafe/libraries/coreinit/coreinit_messagequeue.h"
#include "cafe/libraries/coreinit/coreinit_mutex.h"
#include "cafe/libraries/coreinit/coreinit_scheduler.h"
#include "cafe/libraries/coreinit/coreinit_systeminfo.h"
#include "cafe/libraries/coreinit/coreinit_thread.h"
#include "cafe/libraries/coreinit/coreinit_time.h"
#include "cafe/libraries/tcl/tcl_interrupthandler.h"

#include <atomic>
#include <chrono>
#include <common/log.h>

namespace cafe::gx2
{

using namespace cafe::coreinit;
using namespace cafe::tcl;

struct StaticEventData
{
   struct EventCallbackData
   {
      GX2EventCallbackFunction func;
      virt_ptr<void> data;
   };

   be2_struct<OSThread> appIoThread;
   be2_array<OSMessage, 32> appIoMessageBuffer;
   be2_struct<OSMessageQueue> appIoMessageQueue;
   be2_struct<OSThreadQueue> vsyncThreadQueue;
   be2_struct<OSThreadQueue> flipThreadQueue;
   be2_struct<OSAlarm> vsyncAlarm;
   be2_array<EventCallbackData, GX2EventType::Max> eventCallbacks;

   std::atomic<OSTime> lastVsync;
   std::atomic<OSTime> lastFlip;
   std::atomic<uint32_t> swapCount;
   std::atomic<uint32_t> flipCount;
   std::atomic<uint32_t> framesReady;
};

static virt_ptr<StaticEventData> sEventData = nullptr;

static OSThreadEntryPointFn sAppIoThreadEntry = nullptr;
static AlarmCallbackFn sVsyncAlarmHandler = nullptr;
static TCLInterruptHandlerFn sTclEventCallback = nullptr;

/**
 * Sleep the current thread until the last submitted command buffer
 * has been processed by the driver.
 */
BOOL
GX2DrawDone()
{
   GX2Flush();
   return GX2WaitTimeStamp(GX2GetLastSubmittedTimeStamp());
}


/**
 * Set the callback and data for a type of event.
 */
void
GX2SetEventCallback(GX2EventType type,
                    GX2EventCallbackFunction func,
                    virt_ptr<void> userData)
{
   if (type == GX2EventType::EndOfPipeInterrupt) {
      if (func && !sEventData->eventCallbacks[type].func) {
         TCLIHEnableInterrupt(TCLInterruptType::CP_EOP_EVENT, TRUE);
         TCLIHRegister(TCLInterruptType::CP_EOP_EVENT, sTclEventCallback,
                       virt_cast<void *>(static_cast<virt_addr>(GX2EventType::EndOfPipeInterrupt)));
      } else if (!func && sEventData->eventCallbacks[type].func) {
         TCLIHEnableInterrupt(TCLInterruptType::CP_EOP_EVENT, FALSE);
         TCLIHUnregister(TCLInterruptType::CP_EOP_EVENT, sTclEventCallback,
                         virt_cast<void *>(static_cast<virt_addr>(GX2EventType::EndOfPipeInterrupt)));
      }
   }

   if (type < GX2EventType::Max) {
      sEventData->eventCallbacks[type].func = func;
      sEventData->eventCallbacks[type].data = userData;
   }
}


/**
 * Return the callback and data for a type of event.
 */
void
GX2GetEventCallback(GX2EventType type,
                    virt_ptr<GX2EventCallbackFunction> outFunc,
                    virt_ptr<virt_ptr<void>> outUserData)
{
   if (type < GX2EventType::Max) {
      *outFunc = sEventData->eventCallbacks[type].func;
      *outUserData = sEventData->eventCallbacks[type].data;
   } else {
      *outFunc = nullptr;
      *outUserData = nullptr;
   }
}


/**
 * Return the current swap status.
 *
 * \param outSwapCount
 * Outputs the number of swaps requested.
 *
 * \param outFlipCount
 * Outputs the number of swaps performed.
 *
 * \param outLastFlip
 * Outputs the time of the last flip.
 *
 * \param outLastVsync
 * Outputs the time of the last vsync.
 */
void
GX2GetSwapStatus(virt_ptr<uint32_t> outSwapCount,
                 virt_ptr<uint32_t> outFlipCount,
                 virt_ptr<OSTime> outLastFlip,
                 virt_ptr<OSTime> outLastVsync)
{
   if (outSwapCount) {
      *outSwapCount = sEventData->swapCount.load(std::memory_order_acquire);
   }

   if (outFlipCount) {
      *outFlipCount = sEventData->flipCount.load(std::memory_order_acquire);
   }

   if (outLastFlip) {
      *outLastFlip = sEventData->lastFlip.load(std::memory_order_acquire);
   }

   if (outLastVsync) {
      *outLastVsync = sEventData->lastVsync.load(std::memory_order_acquire);
   }
}


/**
 * Sleep the current thread until the vsync alarm has triggered.
 */
void
GX2WaitForVsync()
{
   OSSleepThread(virt_addrof(sEventData->vsyncThreadQueue));
}


/**
 * Sleep the current thread until a flip has been performed.
 */
void
GX2WaitForFlip()
{
   auto curFlipCount = sEventData->flipCount.load(std::memory_order_acquire);
   auto curSwapCount = sEventData->swapCount.load(std::memory_order_acquire);
   if (curFlipCount == curSwapCount) {
      // The user has no more pending flips, return immediately.
      return;
   }

   OSSleepThread(virt_addrof(sEventData->flipThreadQueue));
}


namespace internal
{

/**
 * TCL interrupt handler which forwards a message to the AppIo thread to
 * perform the user callback.
 */
static void
tclEventCallback(virt_ptr<TCLInterruptEntry> interruptEntry,
                 virt_ptr<void> userData)
{
   auto eventType = static_cast<GX2EventType>(static_cast<uint32_t>(virt_cast<virt_addr>(userData)));
   if (sEventData->eventCallbacks[eventType].func) {
      auto message = StackObject<OSMessage> { };
      message->message = nullptr;
      message->args[0] = eventType;
      message->args[1] = 0u;
      message->args[2] = 0u;
      OSSendMessage(virt_addrof(sEventData->appIoMessageQueue), message,
                    OSMessageFlags::Blocking);
   }
}


/**
 * Initialise GX2 events.
 */
void
initEvents(virt_ptr<void> appIoThreadStackBuffer,
           uint32_t appIoThreadStackSize)
{
   OSInitThreadQueue(virt_addrof(sEventData->flipThreadQueue));
   OSInitThreadQueue(virt_addrof(sEventData->vsyncThreadQueue));

   // Setup 60hz alarm to perform vsync
   auto ticks = static_cast<OSTime>(OSGetSystemInfo()->busSpeed / 4) / 60;
   OSCreateAlarm(virt_addrof(sEventData->vsyncAlarm));
   OSSetPeriodicAlarm(virt_addrof(sEventData->vsyncAlarm), OSGetTime(), ticks,
                      sVsyncAlarmHandler);

   // Create AppIo thread
   OSInitMessageQueue(virt_addrof(sEventData->appIoMessageQueue),
                      virt_addrof(sEventData->appIoMessageBuffer),
                      sEventData->appIoMessageBuffer.size());
   OSCreateThreadType(virt_addrof(sEventData->appIoThread),
                      sAppIoThreadEntry, 0, nullptr,
                      virt_cast<uint32_t *>(virt_cast<virt_addr>(appIoThreadStackBuffer) + appIoThreadStackSize),
                      appIoThreadStackSize,
                      16, OSThreadAttributes::Detached,
                      OSThreadType::AppIo);
   OSResumeThread(virt_addrof(sEventData->appIoThread));

   // Register TCL interrupt handlers.
   TCLIHRegister(TCLInterruptType::CP_IB1, sTclEventCallback,
                 virt_cast<void *>(static_cast<virt_addr>(GX2EventType::StartOfPipeInterrupt)));
   if (sEventData->eventCallbacks[GX2EventType::EndOfPipeInterrupt].func) {
      TCLIHRegister(TCLInterruptType::CP_EOP_EVENT, sTclEventCallback,
                    virt_cast<void *>(static_cast<virt_addr>(GX2EventType::EndOfPipeInterrupt)));
   }
}


/**
 * AppIo thread for GX2, receives messages and calls the appropriate callback.
 */
static uint32_t
appIoThread(uint32_t argc,
            virt_ptr<void> argv)
{
   auto message = StackObject<OSMessage> { };
   while (true) {
      OSReceiveMessage(virt_addrof(sEventData->appIoMessageQueue), message,
                       OSMessageFlags::Blocking);
      auto eventType = static_cast<GX2EventType>(message->args[0]);
      if (eventType == GX2EventType::StopAppIoThread) {
         break;
      }

      auto &callback = sEventData->eventCallbacks[eventType];
      if (callback.func) {
         cafe::invoke(cpu::this_core::state(),
                      callback.func, eventType, callback.data);
      }
   }

   return 0;
}


/**
 * Send a message to stop appIoThread.
 */
void
stopAppIoThread()
{
   // Unregister TCL interrupt handlers.
   TCLIHUnregister(TCLInterruptType::CP_IB1, sTclEventCallback,
                   virt_cast<void *>(static_cast<virt_addr>(GX2EventType::StartOfPipeInterrupt)));
   if (sEventData->eventCallbacks[GX2EventType::EndOfPipeInterrupt].func) {
      TCLIHUnregister(TCLInterruptType::CP_EOP_EVENT, sTclEventCallback,
                      virt_cast<void *>(static_cast<virt_addr>(GX2EventType::EndOfPipeInterrupt)));
   }

   // Send stop message
   auto message = StackObject<OSMessage> { };
   message->message = nullptr;
   message->args[0] = GX2EventType::StopAppIoThread;
   message->args[1] = 0u;
   message->args[2] = 0u;
   OSSendMessage(virt_addrof(sEventData->appIoMessageQueue), message,
                 OSMessageFlags::Blocking);
}


/**
 * VSync alarm handler.
 *
 * Wakes up any threads waiting for vsync.
 * If a vsync callback has been set, trigger it.
 */
static void
vsyncAlarmHandler(virt_ptr<OSAlarm> alarm,
                  virt_ptr<OSContext> context)
{
   auto vsyncTime = OSGetSystemTime();

   auto curFramesReady = sEventData->framesReady.load(std::memory_order_acquire);
   auto curFlipCount = sEventData->flipCount.load(std::memory_order_acquire);
   if (curFramesReady > curFlipCount) {
      sEventData->flipCount.fetch_add(1, std::memory_order_release);
      sEventData->lastFlip.store(vsyncTime, std::memory_order_release);
      OSWakeupThread(virt_addrof(sEventData->flipThreadQueue));

      auto callback = sEventData->eventCallbacks[GX2EventType::Flip];
      if (callback.func) {
         cafe::invoke(cpu::this_core::state(),
                      callback.func,
                      GX2EventType::Flip,
                      callback.data);
      }
   }

   sEventData->lastVsync.store(vsyncTime, std::memory_order_release);
   OSWakeupThread(virt_addrof(sEventData->vsyncThreadQueue));

   auto callback = sEventData->eventCallbacks[GX2EventType::Vsync];
   if (callback.func) {
      cafe::invoke(cpu::this_core::state(),
                   callback.func,
                   GX2EventType::Vsync,
                   callback.data);
   }
}


/**
 * Called when a GX2 command requires more space in a display list.
 *
 * Will call the display list overrun callback to allocate a new command buffer.
 */
std::pair<virt_ptr<void>, uint32_t>
displayListOverrun(virt_ptr<void> list,
                   uint32_t size,
                   uint32_t neededSize)
{
   auto callback = sEventData->eventCallbacks[GX2EventType::DisplayListOverrun];

   if (callback.func && callback.data) {
      auto data = virt_cast<GX2DisplayListOverrunData *>(callback.data);
      data->oldList = list;
      data->oldSize = size;
      data->newList = nullptr;
      data->newSize = neededSize;

      // Call the user's function, it should set newList and newSize
      cafe::invoke(cpu::this_core::state(),
                   callback.func,
                   GX2EventType::DisplayListOverrun,
                   callback.data);
      return { data->newList, data->newSize };
   }

   gLog->error("Encountered DisplayListOverrun without a valid callback!");
   return { nullptr, 0 };
}


/**
 * Called when a swap is requested with GX2SwapBuffers.
 */
void
onSwap()
{
   sEventData->swapCount.fetch_add(1, std::memory_order_release);
}


/**
 * Called when a swap is performed by the driver.
 */
void
onFlip()
{
   sEventData->framesReady.fetch_add(1, std::memory_order_release);
}

} // namespace internal

void
Library::registerEventSymbols()
{
   RegisterFunctionExport(GX2DrawDone);
   RegisterFunctionExport(GX2WaitForVsync);
   RegisterFunctionExport(GX2WaitForFlip);
   RegisterFunctionExport(GX2SetEventCallback);
   RegisterFunctionExport(GX2GetEventCallback);
   RegisterFunctionExport(GX2GetSwapStatus);

   RegisterDataInternal(sEventData);
   RegisterFunctionInternal(internal::vsyncAlarmHandler, sVsyncAlarmHandler);
   RegisterFunctionInternal(internal::appIoThread, sAppIoThreadEntry);
   RegisterFunctionInternal(internal::tclEventCallback, sTclEventCallback);
}

} // namespace cafe::gx2
