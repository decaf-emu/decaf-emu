#pragma once
#include "systemtypes.h"
#include "coreinit_threadqueue.h"
#include "coreinit_time.h"

#pragma pack(push, 1)

struct OSAlarm;
struct OSContext;
using AlarmCallback = wfunc_ptr<void, OSAlarm *, OSContext *>;

struct OSAlarmQueue
{
   static const uint32_t Tag = 0x614C6D51;
   be_val<uint32_t> tag;
   be_ptr<const char> name;
   UNKNOWN(4);

   OSThreadQueue threadQueue;
   be_ptr<OSAlarm> head;
   be_ptr<OSAlarm> tail;
};
CHECK_OFFSET(OSAlarmQueue, 0x00, tag);
CHECK_OFFSET(OSAlarmQueue, 0x04, name);
CHECK_OFFSET(OSAlarmQueue, 0x0c, threadQueue);
CHECK_OFFSET(OSAlarmQueue, 0x1c, head);
CHECK_OFFSET(OSAlarmQueue, 0x20, tail);
CHECK_SIZE(OSAlarmQueue, 0x24);

struct OSAlarm
{
   static const uint32_t Tag = 0x614C724D;
   be_val<uint32_t> tag;
   be_ptr<const char> name;
   UNKNOWN(4);
   AlarmCallback callback;
   be_val<uint32_t> alarmTag; // Like group in memheap
   UNKNOWN(0x38 - 0x14);
   be_ptr<void> userData;
   UNKNOWN(4);
   OSThreadQueue threadQueue;
   be_ptr<OSAlarmQueue> alarmQueue;
   be_ptr<OSContext> context;
};
CHECK_OFFSET(OSAlarm, 0x00, tag);
CHECK_OFFSET(OSAlarm, 0x04, name);
CHECK_OFFSET(OSAlarm, 0x0c, callback);
CHECK_OFFSET(OSAlarm, 0x10, alarmTag);
CHECK_OFFSET(OSAlarm, 0x38, userData);
CHECK_OFFSET(OSAlarm, 0x40, threadQueue);
CHECK_OFFSET(OSAlarm, 0x50, alarmQueue);
CHECK_OFFSET(OSAlarm, 0x54, context);
CHECK_SIZE(OSAlarm, 0x58);

#pragma pack(pop)

BOOL
OSCancelAlarm(OSAlarm *alarm);

void
OSCancelAlarms(uint32_t alarmTag);

void
OSCreateAlarm(OSAlarm *alarm);

void
OSCreateAlarmEx(OSAlarm *alarm, const char *name);

void *
OSGetAlarmUserData(OSAlarm *alarm);

BOOL
OSSetAlarm(OSAlarm *alarm, Time time, AlarmCallback callback);

BOOL
OSSetPeriodicAlarm(OSAlarm *alarm, Time start, Time interval, AlarmCallback callback);

void
OSSetAlarmTag(OSAlarm *alarm, uint32_t alarmTag);

void
OSSetAlarmUserData(OSAlarm *alarm, void *data);

BOOL
OSWaitAlarm(OSAlarm *alarm);
