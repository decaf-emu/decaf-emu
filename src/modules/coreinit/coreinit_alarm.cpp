#include "coreinit.h"
#include "coreinit_alarm.h"

const uint32_t OSAlarm::Tag;

BOOL
OSCancelAlarm(OSAlarm *alarm)
{
   return FALSE;
}

void
OSCancelAlarms(uint32_t alarmTag)
{
}

void
OSCreateAlarm(OSAlarm *alarm)
{
   OSCreateAlarmEx(alarm, nullptr);
}

void
OSCreateAlarmEx(OSAlarm *alarm, const char *name)
{
   memset(alarm, 0, sizeof(OSAlarm));
   alarm->tag = OSAlarm::Tag;
   alarm->name = name;
   OSInitThreadQueueEx(&alarm->threadQueue, alarm);
}

void *
OSGetAlarmUserData(OSAlarm *alarm)
{
   return alarm->userData;
}

BOOL
OSSetAlarm(OSAlarm *alarm, Time time, AlarmCallback callback)
{
   return FALSE;
}

BOOL
OSSetPeriodicAlarm(OSAlarm *alarm, Time start, Time interval, AlarmCallback callback)
{
   return FALSE;
}

void
OSSetAlarmTag(OSAlarm *alarm, uint32_t alarmTag)
{
   alarm->alarmTag = alarmTag;
}

void
OSSetAlarmUserData(OSAlarm *alarm, void *data)
{
   alarm->userData = data;
}

BOOL
OSWaitAlarm(OSAlarm *alarm)
{
   return FALSE;
}

void
CoreInit::registerAlarmFunctions()
{
   RegisterKernelFunction(OSCancelAlarm);
   RegisterKernelFunction(OSCancelAlarms);
   RegisterKernelFunction(OSCreateAlarm);
   RegisterKernelFunction(OSCreateAlarmEx);
   RegisterKernelFunction(OSGetAlarmUserData);
   RegisterKernelFunction(OSSetAlarm);
   RegisterKernelFunction(OSSetPeriodicAlarm);
   RegisterKernelFunction(OSSetAlarmTag);
   RegisterKernelFunction(OSSetAlarmUserData);
   RegisterKernelFunction(OSWaitAlarm);
}
