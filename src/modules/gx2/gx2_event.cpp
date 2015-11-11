#include "gx2.h"
#include "gx2_event.h"
#include "modules/coreinit/coreinit_alarm.h"
#include "modules/coreinit/coreinit_memheap.h"
#include "modules/coreinit/coreinit_systeminfo.h"
#include "modules/coreinit/coreinit_thread.h"
#include "modules/coreinit/coreinit_threadqueue.h"
#include "modules/coreinit/coreinit_time.h"
#include "utils/wfunc_call.h"
#include <chrono>

static OSTime
gLastVsync = 0;

static OSTime
gLastSubmittedTimestamp = 0;

static OSTime
gRetiredTimestamp = 0;

static virtual_ptr<OSThreadQueue>
gVsyncThreadQueue;

static virtual_ptr<OSAlarm>
gVsyncAlarm;

static AlarmCallback
pVsyncAlarmHandler = nullptr;

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
   return gRetiredTimestamp;
}


OSTime
GX2GetLastSubmittedTimeStamp()
{
   return gLastSubmittedTimestamp;
}


BOOL
GX2WaitTimeStamp(OSTime time)
{
   // TODO: Sleep until GPU has processed TimeStamp
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

   auto ticks = static_cast<OSTime>(OSGetSystemInfo()->clockSpeed / 4) / 60;
   OSCreateAlarm(gVsyncAlarm);
   OSSetPeriodicAlarm(gVsyncAlarm, OSGetTime(), ticks, pVsyncAlarmHandler);
}


void
vsyncAlarmHandler(OSAlarm *alarm, OSContext *context)
{
   gLastVsync = OSGetSystemTime();
   OSWakeupThread(gVsyncThreadQueue);
   auto callback = gEventCallbacks[GX2EventType::Vsync];

   if (callback.func) {
      callback.func(GX2EventType::Vsync, callback.data);
   }
}


void
setRetiredTimestamp(OSTime timestamp)
{
   gRetiredTimestamp = timestamp;
}


void
setLastSubmittedTimestamp(OSTime timestamp)
{
   gLastSubmittedTimestamp = timestamp;
}

} // namespace internal

} // namespace gx2


void
GX2::initialiseVsync()
{
   pVsyncAlarmHandler = findExportAddress("VsyncAlarmHandler");
}
