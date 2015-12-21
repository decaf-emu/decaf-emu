#include "gx2.h"
#include "gx2_event.h"
#include "modules/coreinit/coreinit_alarm.h"
#include "modules/coreinit/coreinit_memheap.h"
#include "modules/coreinit/coreinit_mutex.h"
#include "modules/coreinit/coreinit_scheduler.h"
#include "modules/coreinit/coreinit_systeminfo.h"
#include "modules/coreinit/coreinit_thread.h"
#include "modules/coreinit/coreinit_threadqueue.h"
#include "modules/coreinit/coreinit_time.h"
#include "utils/wfunc_call.h"
#include <atomic>
#include <chrono>

static std::atomic<int64_t>
gLastVsync { 0 };

static std::atomic<int64_t>
gLastFlip { 0 };

static std::atomic<uint32_t>
gSwapCount { 0 };

static std::atomic<uint32_t>
gFlipCount { 0 };

static std::atomic<int64_t>
gLastSubmittedTimestamp { 0 };

static std::atomic<int64_t>
gRetiredTimestamp { 0 };

static virtual_ptr<OSThreadQueue>
gVsyncThreadQueue;

static virtual_ptr<OSThreadQueue>
gFlipThreadQueue;

static virtual_ptr<OSAlarm>
gVsyncAlarm;

static AlarmCallback
pVsyncAlarmHandler = nullptr;

static virtual_ptr<OSThreadQueue>
gWaitTimeStampQueue = nullptr;

struct EventCallbackData
{
   GX2EventCallbackFunction func;
   void *data;
};

static EventCallbackData
gEventCallbacks[GX2EventType::Max];


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
      gEventCallbacks[type].func = func;
      gEventCallbacks[type].data = userData;
   }
}


/**
 * Return the callback and data for a type of event.
 */
void
GX2GetEventCallback(GX2EventType type,
                    be_GX2EventCallbackFunction *funcOut,
                    be_ptr<void> *userDataOut)
{
   if (type < GX2EventType::Max) {
      *funcOut = gEventCallbacks[type].func;
      *userDataOut = gEventCallbacks[type].data;
   } else {
      *funcOut = 0;
      *userDataOut = 0;
   }
}


/**
 * Get the timestamp of the last command buffer processed by the driver.
 */
OSTime
GX2GetRetiredTimeStamp()
{
   return gRetiredTimestamp.load(std::memory_order_acquire);
}


/**
 * Get the timestamp of the last submitted command buffer.
 */
OSTime
GX2GetLastSubmittedTimeStamp()
{
   return gLastSubmittedTimestamp.load(std::memory_order_acquire);
}


/**
 * Return the current swap status.
 *
 * swapCount is the number of swaps requested
 * flipCount is the number of swaps performed
 * lastFlip is the timestamp of the last flip
 * lastVsync is the timestamp of the last vsync
 */
void
GX2GetSwapStatus(be_val<uint32_t> *swapCount,
                 be_val<uint32_t> *flipCount,
                 be_val<OSTime> *lastFlip,
                 be_val<OSTime> *lastVsync)
{
   *swapCount = gSwapCount.load(std::memory_order_acquire);
   *flipCount = gFlipCount.load(std::memory_order_acquire);

   *lastFlip = gLastFlip.load(std::memory_order_acquire);
   *lastVsync = gLastVsync.load(std::memory_order_acquire);
}


/**
 * Wait until a command buffer submitted with a timestamp has been processed by the driver.
 */
BOOL
GX2WaitTimeStamp(OSTime time)
{
   OSLockScheduler();

   while (gRetiredTimestamp.load(std::memory_order_acquire) < time) {
      OSSleepThreadNoLock(gWaitTimeStampQueue);
      OSRescheduleNoLock();
   }

   OSUnlockScheduler();
   return TRUE;
}


/**
 * Sleep the current thread until the vsync alarm has triggered.
 */
void
GX2WaitForVsync()
{
   OSSleepThread(gVsyncThreadQueue);
}


/**
 * Sleep the current thread until a flip has been performed.
 */
void
GX2WaitForFlip()
{
   OSSleepThread(gFlipThreadQueue);
}


namespace gx2
{

namespace internal
{


/**
 * Start a vsync alarm.
 */
void
initEvents()
{
   gVsyncThreadQueue = OSAllocFromSystem<OSThreadQueue>();
   gVsyncAlarm = OSAllocFromSystem<OSAlarm>();
   gWaitTimeStampQueue = OSAllocFromSystem<OSThreadQueue>();
   gFlipThreadQueue = OSAllocFromSystem<OSThreadQueue>();

   auto ticks = static_cast<OSTime>(OSGetSystemInfo()->clockSpeed / 4) / 60;
   OSCreateAlarm(gVsyncAlarm);
   OSSetPeriodicAlarm(gVsyncAlarm, OSGetTime(), ticks, pVsyncAlarmHandler);
}


/**
 * VSync alarm handler.
 *
 * Wakes up any threads waiting for vsync.
 * If a vsync callback has been set, trigger it.
 */
void
vsyncAlarmHandler(OSAlarm *alarm, OSContext *context)
{
   gLastVsync.store(OSGetSystemTime(), std::memory_order_release);
   OSWakeupThreadNoLock(gVsyncThreadQueue);
   auto callback = gEventCallbacks[GX2EventType::Vsync];

   if (callback.func) {
      OSUnlockScheduler();
      callback.func(GX2EventType::Vsync, callback.data);
      OSLockScheduler();
   }
}


/**
 * Called when a GX2 command requires more space in a display list.
 *
 * Will call the display list overrun callback to allocate a new command buffer.
 */
std::pair<void *, uint32_t>
displayListOverrun(void *list, uint32_t size)
{
   auto callback = gEventCallbacks[GX2EventType::DisplayListOverrun];
   void *newList = nullptr;
   uint32_t newSize = 0u;

   if (callback.func && callback.data) {
      auto data = reinterpret_cast<GX2DisplayListOverrunData *>(callback.data);
      data->oldList = list;
      data->oldSize = size;
      data->newList = nullptr;
      data->newSize = 0;

      // Call the user's function, it should set newList and newSize
      callback.func(GX2EventType::DisplayListOverrun, callback.data);
      return { data->newList, data->newSize };
   }

   gLog->error("Encountered DisplayListOverrun without a valid callback!");
   return { nullptr, 0 };
}


/**
 * Update the retired timestamp.
 *
 * Will wakeup any threads waiting for a timestamp.
 */
void
setRetiredTimestamp(OSTime timestamp)
{
   OSLockScheduler();
   gRetiredTimestamp.store(timestamp, std::memory_order_release);
   OSWakeupThreadNoLock(gWaitTimeStampQueue);
   OSUnlockScheduler();
}


/**
 * Update the last submitted timestamp.
 */
void
setLastSubmittedTimestamp(OSTime timestamp)
{
   gLastSubmittedTimestamp.store(timestamp, std::memory_order_release);
}


/**
 * Called when a swap is requested with GX2SwapBuffers.
 */
void
onSwap()
{
   gSwapCount++;
}


/**
 * Called when a swap is performed by the driver.
 */
void
onFlip()
{
   gFlipCount++;
   gLastFlip.store(OSGetSystemTime(), std::memory_order_release);
   OSWakeupThread(gFlipThreadQueue);
}

} // namespace internal

} // namespace gx2


void
GX2::initialiseVsync()
{
   pVsyncAlarmHandler = findExportAddress("VsyncAlarmHandler");
}
