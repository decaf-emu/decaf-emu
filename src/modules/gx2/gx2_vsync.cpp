#include "modules/gx2/gx2.h"
#include "modules/gx2/gx2_vsync.h"
#include "modules/coreinit/coreinit_alarm.h"
#include "modules/coreinit/coreinit_memheap.h"
#include "modules/coreinit/coreinit_systeminfo.h"
#include "modules/coreinit/coreinit_thread.h"
#include "modules/coreinit/coreinit_threadqueue.h"
#include "modules/coreinit/coreinit_time.h"
#include "interpreter.h"
#include <chrono>

OSTime
gLastVsync = 0;

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
gEventCallbacks[GX2EventCallback::Max];

void
_GX2InitVsync()
{
   gVsyncThreadQueue = OSAllocFromSystem<OSThreadQueue>();
   gVsyncAlarm = OSAllocFromSystem<OSAlarm>();

   auto ticks = (static_cast<OSTime>(OSGetSystemInfo()->clockSpeed / 4) * 60) / 1000;
   OSCreateAlarm(gVsyncAlarm);
   OSSetPeriodicAlarm(gVsyncAlarm, 0, OSGetTime(), ticks, pVsyncAlarmHandler);
}

void
GX2SetEventCallback(GX2EventCallback::Type type, GX2EventCallbackFunction func, void *userData)
{
   if (type < GX2EventCallback::Max) {
      gEventCallbacks[type].func = func;
      gEventCallbacks[type].data = userData;
   }
}

void
GX2GetEventCallback(GX2EventCallback::Type type, be_GX2EventCallbackFunction *funcOut, be_ptr<void> *userDataOut)
{
   if (type < GX2EventCallback::Max) {
      *funcOut = gEventCallbacks[type].func;
      *userDataOut = gEventCallbacks[type].data;
   } else {
      *funcOut = 0;
      *userDataOut = 0;
   }
}

void
GX2WaitForVsync()
{
   OSSleepThread(gVsyncThreadQueue);
}

void
VsyncAlarmHandler(OSAlarm *alarm, OSContext *context)
{
   gLastVsync = OSGetSystemTime();
   OSWakeupThread(gVsyncThreadQueue);
   auto callback = gEventCallbacks[GX2EventCallback::Vsync];

   if (callback.func) {
      callback.func(GX2EventCallback::Vsync, callback.data);
   }
}

void
GX2::initialiseVsync()
{
   pVsyncAlarmHandler = findExportAddress("VsyncAlarmHandler");
}

void
GX2::registerVsyncFunctions()
{
   RegisterKernelFunction(GX2WaitForVsync);
   RegisterKernelFunction(GX2SetEventCallback);
   RegisterKernelFunction(GX2GetEventCallback);
   RegisterKernelFunction(VsyncAlarmHandler);
}
