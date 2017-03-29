#include "gx2.h"
#include "gx2_event.h"
#include "gx2_state.h"
#include "modules/coreinit/coreinit_alarm.h"
#include "modules/coreinit/coreinit_memheap.h"
#include "modules/coreinit/coreinit_mutex.h"
#include "modules/coreinit/coreinit_scheduler.h"
#include "modules/coreinit/coreinit_systeminfo.h"
#include "modules/coreinit/coreinit_thread.h"
#include "modules/coreinit/coreinit_time.h"
#include "ppcutils/wfunc_call.h"
#include <atomic>
#include <chrono>

using namespace coreinit;

namespace gx2
{

static std::atomic<int64_t>
sLastVsync { 0 };

static std::atomic<int64_t>
sLastFlip { 0 };

static std::atomic<uint32_t>
sSwapCount { 0 };

static std::atomic<uint32_t>
sFlipCount { 0 };

static std::atomic<uint32_t>
sFramesReady { 0 };

static std::atomic<int64_t>
sLastSubmittedTimestamp { 0 };

static std::atomic<int64_t>
sRetiredTimestamp { 0 };

static OSThreadQueue *
sVsyncThreadQueue = nullptr;

static OSThreadQueue *
sFlipThreadQueue = nullptr;

static OSAlarm *
sVsyncAlarm = nullptr;

static AlarmCallback
sVsyncAlarmHandler = nullptr;

static OSThreadQueue *
sWaitTimeStampQueue = nullptr;

struct EventCallbackData
{
   GX2EventCallbackFunction func;
   void *data;
};

static EventCallbackData
sEventCallbacks[GX2EventType::Max];


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
                    void *userData)
{
   if (type == GX2EventType::DisplayListOverrun && !userData) {
      gLog->error("DisplayListOverrun callback set with no valid userData");
   }

   if (type < GX2EventType::Max) {
      sEventCallbacks[type].func = func;
      sEventCallbacks[type].data = userData;
   }
}


/**
 * Return the callback and data for a type of event.
 */
void
GX2GetEventCallback(GX2EventType type,
                    GX2EventCallbackFunction::be *funcOut,
                    be_ptr<void> *userDataOut)
{
   if (type < GX2EventType::Max) {
      *funcOut = sEventCallbacks[type].func;
      *userDataOut = sEventCallbacks[type].data;
   } else {
      *funcOut = nullptr;
      *userDataOut = nullptr;
   }
}


/**
 * Get the timestamp of the last command buffer processed by the driver.
 */
OSTime
GX2GetRetiredTimeStamp()
{
   return sRetiredTimestamp.load(std::memory_order_acquire);
}


/**
 * Get the timestamp of the last submitted command buffer.
 */
OSTime
GX2GetLastSubmittedTimeStamp()
{
   return sLastSubmittedTimestamp.load(std::memory_order_acquire);
}


/**
 * Return the current swap status.
 *
 * \param swapCount
 * Outputs the number of swaps requested.
 *
 * \param flipCount
 * Outputs the number of swaps performed.
 *
 * \param lastFlip
 * Outputs the timestamp of the last flip.
 *
 * \param lastVsync
 * Outputs the timestamp of the last vsync.
 */
void
GX2GetSwapStatus(be_val<uint32_t> *swapCount,
                 be_val<uint32_t> *flipCount,
                 be_val<OSTime> *lastFlip,
                 be_val<OSTime> *lastVsync)
{
   if (swapCount) {
      *swapCount = sSwapCount.load(std::memory_order_acquire);
   }

   if (flipCount) {
      *flipCount = sFlipCount.load(std::memory_order_acquire);
   }

   if (lastFlip) {
      *lastFlip = sLastFlip.load(std::memory_order_acquire);
   }

   if (lastVsync) {
      *lastVsync = sLastVsync.load(std::memory_order_acquire);
   }
}


/**
 * Wait until a command buffer submitted with a timestamp has been processed by the driver.
 */
BOOL
GX2WaitTimeStamp(OSTime time)
{
   coreinit::internal::lockScheduler();

   while (sRetiredTimestamp.load(std::memory_order_acquire) < time) {
      coreinit::internal::sleepThreadNoLock(sWaitTimeStampQueue);
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
   OSSleepThread(sVsyncThreadQueue);
}


/**
 * Sleep the current thread until a flip has been performed.
 */
void
GX2WaitForFlip()
{
   if (sFlipCount == sSwapCount) {
      // The user has no more pending flips, return immediately.
      return;
   }

   OSSleepThread(sFlipThreadQueue);
}


namespace internal
{


/**
 * Start a vsync alarm.
 */
void
initEvents()
{
   sVsyncThreadQueue = coreinit::internal::sysAlloc<OSThreadQueue>();
   sVsyncAlarm = coreinit::internal::sysAlloc<OSAlarm>();
   sWaitTimeStampQueue = coreinit::internal::sysAlloc<OSThreadQueue>();
   sFlipThreadQueue = coreinit::internal::sysAlloc<OSThreadQueue>();

   auto ticks = static_cast<OSTime>(OSGetSystemInfo()->busSpeed / 4) / 60;
   OSCreateAlarm(sVsyncAlarm);
   OSSetPeriodicAlarm(sVsyncAlarm, OSGetTime(), ticks, sVsyncAlarmHandler);
}


/**
 * VSync alarm handler.
 *
 * Wakes up any threads waiting for vsync.
 * If a vsync callback has been set, trigger it.
 */
static void
vsyncAlarmHandler(OSAlarm *alarm, OSContext *context)
{
   auto vsyncTime = OSGetSystemTime();

   if (sFramesReady > sFlipCount) {
      sFlipCount++;
      sLastFlip.store(vsyncTime, std::memory_order_release);
      OSWakeupThread(sFlipThreadQueue);
   }

   sLastVsync.store(vsyncTime, std::memory_order_release);
   OSWakeupThread(sVsyncThreadQueue);

   auto callback = sEventCallbacks[GX2EventType::Vsync];

   if (callback.func) {
      callback.func(GX2EventType::Vsync, callback.data);
   }
}


/**
 * Called when a GX2 command requires more space in a display list.
 *
 * Will call the display list overrun callback to allocate a new command buffer.
 */
std::pair<void *, uint32_t>
displayListOverrun(void *list, uint32_t size, uint32_t neededSize)
{
   auto callback = sEventCallbacks[GX2EventType::DisplayListOverrun];

   if (callback.func && callback.data) {
      auto data = reinterpret_cast<GX2DisplayListOverrunData *>(callback.data);
      data->oldList = list;
      data->oldSize = size;
      data->newList = nullptr;
      data->newSize = neededSize;

      // Call the user's function, it should set newList and newSize
      callback.func(GX2EventType::DisplayListOverrun, callback.data);
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
   OSWakeupThread(sWaitTimeStampQueue);
}


/**
 * Update the retired timestamp.
 */
void
setRetiredTimestamp(OSTime timestamp)
{
   sRetiredTimestamp.store(timestamp, std::memory_order_release);
   cpu::interrupt(gx2::internal::getMainCoreId(), cpu::GPU_RETIRE_INTERRUPT);
}


/**
 * Update the last submitted timestamp.
 */
void
setLastSubmittedTimestamp(OSTime timestamp)
{
   sLastSubmittedTimestamp.store(timestamp, std::memory_order_release);
}


/**
 * Called when a swap is requested with GX2SwapBuffers.
 */
void
onSwap()
{
   sSwapCount++;
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
   sFramesReady++;
   cpu::interrupt(gx2::internal::getMainCoreId(), cpu::GPU_FLIP_INTERRUPT);
}

} // namespace internal

void
Module::RegisterVsyncFunctions()
{
   RegisterInternalFunction(internal::vsyncAlarmHandler, sVsyncAlarmHandler);
}

} // namespace gx2
