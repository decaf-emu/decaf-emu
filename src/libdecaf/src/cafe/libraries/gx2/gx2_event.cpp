#include "gx2.h"
#include "gx2_event.h"
#include "gx2_state.h"
#include "cafe/libraries/coreinit/coreinit_alarm.h"
#include "cafe/libraries/coreinit/coreinit_memheap.h"
#include "cafe/libraries/coreinit/coreinit_mutex.h"
#include "cafe/libraries/coreinit/coreinit_scheduler.h"
#include "cafe/libraries/coreinit/coreinit_systeminfo.h"
#include "cafe/libraries/coreinit/coreinit_thread.h"
#include "cafe/libraries/coreinit/coreinit_time.h"
#include "cafe/cafe_ppc_interface_invoke.h"

#include <atomic>
#include <chrono>
#include <common/log.h>

using namespace cafe::coreinit;

namespace cafe::gx2
{

struct StaticEventData
{
   struct EventCallbackData
   {
      GX2EventCallbackFunction func;
      virt_ptr<void> data;
   };

   std::atomic<int64_t> lastVsync;
   std::atomic<int64_t> lastFlip;
   std::atomic<int64_t> lastSubmittedTimestamp;
   std::atomic<int64_t> retiredTimestamp;
   std::atomic<uint32_t> swapCount;
   std::atomic<uint32_t> flipCount;
   std::atomic<uint32_t> framesReady;
   be2_struct<OSThreadQueue> vsyncThreadQueue;
   be2_struct<OSThreadQueue> flipThreadQueue;
   be2_struct<OSThreadQueue> waitTimeStampQueue;
   be2_struct<OSAlarm> vsyncAlarm;
   be2_array<EventCallbackData, GX2EventType::Max> eventCallbacks;
};

static virt_ptr<StaticEventData> sEventData = nullptr;
static AlarmCallbackFn VsyncAlarmHandler = nullptr;


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
   if (type == GX2EventType::DisplayListOverrun && !userData) {
      gLog->error("DisplayListOverrun callback set with no valid userData");
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
 * Get the timestamp of the last command buffer processed by the driver.
 */
GX2Timestamp
GX2GetRetiredTimeStamp()
{
   return sEventData->retiredTimestamp.load(std::memory_order_acquire);
}


/**
 * Get the timestamp of the last submitted command buffer.
 */
GX2Timestamp
GX2GetLastSubmittedTimeStamp()
{
   return sEventData->lastSubmittedTimestamp.load(std::memory_order_acquire);
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
 * Outputs the timestamp of the last flip.
 *
 * \param outLastVsync
 * Outputs the timestamp of the last vsync.
 */
void
GX2GetSwapStatus(virt_ptr<uint32_t> outSwapCount,
                 virt_ptr<uint32_t> outFlipCount,
                 virt_ptr<GX2Timestamp> outLastFlip,
                 virt_ptr<GX2Timestamp> outLastVsync)
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
 * Wait until a command buffer submitted with a timestamp has been processed by the driver.
 */
BOOL
GX2WaitTimeStamp(GX2Timestamp time)
{
   coreinit::internal::lockScheduler();

   while (sEventData->retiredTimestamp.load(std::memory_order_acquire) < time) {
      coreinit::internal::sleepThreadNoLock(virt_addrof(sEventData->waitTimeStampQueue));
      coreinit::internal::rescheduleSelfNoLock();
   }

   coreinit::internal::unlockScheduler();
   return TRUE;
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
   if (sEventData->flipCount == sEventData->swapCount) {
      // The user has no more pending flips, return immediately.
      return;
   }

   OSSleepThread(virt_addrof(sEventData->flipThreadQueue));
}


namespace internal
{


/**
 * Start a vsync alarm.
 */
void
initEvents()
{
   OSInitThreadQueue(virt_addrof(sEventData->flipThreadQueue));
   OSInitThreadQueue(virt_addrof(sEventData->vsyncThreadQueue));
   OSInitThreadQueue(virt_addrof(sEventData->waitTimeStampQueue));

   auto ticks = static_cast<OSTime>(OSGetSystemInfo()->busSpeed / 4) / 60;
   OSCreateAlarm(virt_addrof(sEventData->vsyncAlarm));
   OSSetPeriodicAlarm(virt_addrof(sEventData->vsyncAlarm), OSGetTime(), ticks,
                      VsyncAlarmHandler);
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

   if (sEventData->framesReady > sEventData->flipCount) {
      sEventData->flipCount++;
      sEventData->lastFlip.store(vsyncTime, std::memory_order_release);
      OSWakeupThread(virt_addrof(sEventData->flipThreadQueue));
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
 * This is called by the GPU retire interrupt handler.
 * Will wakeup any threads waiting for a timestamp.
 */
void
handleGpuRetireInterrupt()
{
   OSWakeupThread(virt_addrof(sEventData->waitTimeStampQueue));
}


/**
 * Update the retired timestamp.
 */
void
setRetiredTimestamp(GX2Timestamp timestamp)
{
   sEventData->retiredTimestamp.store(timestamp, std::memory_order_release);
   cpu::interrupt(gx2::internal::getMainCoreId(), cpu::GPU_RETIRE_INTERRUPT);
}


/**
 * Update the last submitted timestamp.
 */
void
setLastSubmittedTimestamp(GX2Timestamp timestamp)
{
   sEventData->lastSubmittedTimestamp.store(timestamp, std::memory_order_release);
}


/**
 * Called when a swap is requested with GX2SwapBuffers.
 */
void
onSwap()
{
   sEventData->swapCount++;
}


/**
 * This is called by the GPU flip interrupt handler.
 * Will wakeup any threads waiting for a flip.
 */
void
handleGpuFlipInterrupt()
{
}


/**
 * Called when a swap is performed by the driver.
 */
void
onFlip()
{
   sEventData->framesReady++;
   cpu::interrupt(gx2::internal::getMainCoreId(), cpu::GPU_FLIP_INTERRUPT);
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
   RegisterFunctionExport(GX2GetRetiredTimeStamp);
   RegisterFunctionExport(GX2GetLastSubmittedTimeStamp);
   RegisterFunctionExport(GX2WaitTimeStamp);
   RegisterFunctionExport(GX2GetSwapStatus);

   RegisterDataInternal(sEventData);
   RegisterFunctionInternal(internal::vsyncAlarmHandler, VsyncAlarmHandler);
}

} // namespace cafe::gx2
