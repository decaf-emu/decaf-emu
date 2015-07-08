#include "coreinit.h"
#include "coreinit_alarm.h"

BOOL
OSCancelAlarm(OSAlarm *alarm)
{
   assert(false);
   return FALSE;
}

void
OSCancelAlarms(uint32_t alarmTag)
{
   assert(false);
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
   assert(false);
   return FALSE;
}

BOOL
OSSetPeriodicAlarm(OSAlarm *alarm, Time start, Time interval, AlarmCallback callback)
{
   assert(false);
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
   assert(false);
   return FALSE;
}
