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

static std::atomic_int64_t
gLastVsync = 0;

static std::atomic_int64_t
gLastSubmittedTimestamp = 0;

static std::atomic_int64_t
gRetiredTimestamp = 0;

static virtual_ptr<OSThreadQueue>
gVsyncThreadQueue;

static virtual_ptr<OSAlarm>
gVsyncAlarm;

static AlarmCallback
pVsyncAlarmHandler = nullptr;

static virtual_ptr<OSThreadQueue>
gWaitTimeStampQueue = nullptr;

struct EventCallbackData
{
   GX2EventCallbackFunction func;
   virtual_ptr<void> data;
};

static EventCallbackData
gEventCallbacks[GX2EventType::Last];


BOOL
GX2DrawDone()
{
   return GX2WaitTimeStamp(GX2GetLastSubmittedTimeStamp());
}


void
GX2WaitForVsync()
{
   OSSleepThread(gVsyncThreadQueue);
}


void
GX2WaitForFlip()
{
   // TODO: What is a flip? :D
}


void
GX2SetEventCallback(GX2EventType::Value type, GX2EventCallbackFunction func, void *userData)
{
   if (type < GX2EventType::Last) {
      gEventCallbacks[type].func = func;
      gEventCallbacks[type].data = userData;
   }
}


void
GX2GetEventCallback(GX2EventType::Value type, be_GX2EventCallbackFunction *funcOut, be_ptr<void> *userDataOut)
{
   if (type < GX2EventType::Last) {
      *funcOut = gEventCallbacks[type].func;
      *userDataOut = gEventCallbacks[type].data;
   } else {
      *funcOut = 0;
      *userDataOut = 0;
   }
}


OSTime
GX2GetRetiredTimeStamp()
{
   return gRetiredTimestamp.load(std::memory_order_acquire);
}


OSTime
GX2GetLastSubmittedTimeStamp()
{
   return gLastSubmittedTimestamp.load(std::memory_order_acquire);
}


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


namespace gx2
{

namespace internal
{


void
initVsync()
{
   gVsyncThreadQueue = OSAllocFromSystem<OSThreadQueue>();
   gVsyncAlarm = OSAllocFromSystem<OSAlarm>();
   gWaitTimeStampQueue = OSAllocFromSystem<OSThreadQueue>();

   auto ticks = static_cast<OSTime>(OSGetSystemInfo()->clockSpeed / 4) / 60;
   OSCreateAlarm(gVsyncAlarm);
   OSSetPeriodicAlarm(gVsyncAlarm, OSGetTime(), ticks, pVsyncAlarmHandler);
}


void
vsyncAlarmHandler(OSAlarm *alarm, OSContext *context)
{
   gLastVsync.store(OSGetSystemTime(), std::memory_order_release);
   OSWakeupThread(gVsyncThreadQueue);
   auto callback = gEventCallbacks[GX2EventType::Vsync];

   if (callback.func) {
      callback.func(GX2EventType::Vsync, callback.data);
   }
}


void
setRetiredTimestamp(OSTime timestamp)
{
   // TODO: Is there a non-race way to do this without acquiring scheduler lock?
   OSLockScheduler();
   gRetiredTimestamp.store(timestamp, std::memory_order_release);
   OSWakeupThreadNoLock(gWaitTimeStampQueue);
   OSUnlockScheduler();
}


void
setLastSubmittedTimestamp(OSTime timestamp)
{
   gLastSubmittedTimestamp.store(timestamp, std::memory_order_release);
}

} // namespace internal

} // namespace gx2


void
GX2::initialiseVsync()
{
   pVsyncAlarmHandler = findExportAddress("VsyncAlarmHandler");
}
