#pragma once
#include "types.h"
#include "coreinit_enum.h"
#include "coreinit_thread.h"
#include "coreinit_time.h"
#include "common/be_val.h"
#include "common/structsize.h"
#include "common/virtual_ptr.h"
#include "ppcutils/wfunc_ptr.h"

namespace coreinit
{

/**
 * \defgroup coreinit_alarms Alarms
 * \ingroup coreinit
 * @{
 */

#pragma pack(push, 1)

struct OSAlarm;
struct OSContext;
using AlarmCallback = wfunc_ptr<void, OSAlarm *, OSContext *>;
using be_AlarmCallback = be_wfunc_ptr<void, OSAlarm *, OSContext *>;

struct OSAlarmQueue
{
   static const uint32_t Tag = 0x614C6D51;

   //! Should always be set to the value OSAlarmQueue::Tag.
   be_val<uint32_t> tag;

   //! Name set from OSInitAlarmQueueEx.
   be_ptr<const char> name;

   UNKNOWN(4);

   //! List of threads waiting on this alarm queue.
   OSThreadQueue threadQueue;

   //! First alarm in the queue.
   be_ptr<OSAlarm> head;

   //! Last alarm in the queue.
   be_ptr<OSAlarm> tail;
};
CHECK_OFFSET(OSAlarmQueue, 0x00, tag);
CHECK_OFFSET(OSAlarmQueue, 0x04, name);
CHECK_OFFSET(OSAlarmQueue, 0x0c, threadQueue);
CHECK_OFFSET(OSAlarmQueue, 0x1c, head);
CHECK_OFFSET(OSAlarmQueue, 0x20, tail);
CHECK_SIZE(OSAlarmQueue, 0x24);

struct OSAlarmLink
{
   //! Previous alarm in the queue.
   be_ptr<OSAlarm> prev;

   //! Next alarm in the queue.
   be_ptr<OSAlarm> next;
};
CHECK_OFFSET(OSAlarmLink, 0x00, prev);
CHECK_OFFSET(OSAlarmLink, 0x04, next);
CHECK_SIZE(OSAlarmLink, 0x08);

struct OSAlarm
{
   static const uint32_t Tag = 0x614C724D;

   //! Should always be set to the value OSAlarm::Tag.
   be_val<uint32_t> tag;

   //! Name set from OSCreateAlarmEx.
   be_ptr<const char> name;

   UNKNOWN(4);

   //! The callback to execute once the alarm is triggered.
   be_AlarmCallback callback;

   //! Used with OSCancelAlarms for bulk cancellation of alarms.
   be_val<uint32_t> group;

   UNKNOWN(4);

   //! The time when the alarm will next be triggered.
   be_val<OSTime> nextFire;

   //! Link used for when this OSAlarm object is inside an OSAlarmQueue
   OSAlarmLink link;

   //! The period between alarm triggers, this is only set for periodic alarms.
   be_val<OSTime> period;

   //! The time the alarm was started.
   be_val<OSTime> tbrStart;

   //! User data set with OSSetAlarmUserData and retrieved with OSGetAlarmUserData.
   be_ptr<void> userData;

   //! The current state of the alarm.
   be_val<OSAlarmState> state;

   //! Queue of threads currently waiting for the alarm to trigger with OSWaitAlarm.
   OSThreadQueue threadQueue;

   //! The queue that this alarm is currently in.
   be_ptr<OSAlarmQueue> alarmQueue;

   //! The context the alarm was triggered on.
   be_ptr<OSContext> context;
};
CHECK_OFFSET(OSAlarm, 0x00, tag);
CHECK_OFFSET(OSAlarm, 0x04, name);
CHECK_OFFSET(OSAlarm, 0x0c, callback);
CHECK_OFFSET(OSAlarm, 0x10, group);
CHECK_OFFSET(OSAlarm, 0x18, nextFire);
CHECK_OFFSET(OSAlarm, 0x20, link);
CHECK_OFFSET(OSAlarm, 0x28, period);
CHECK_OFFSET(OSAlarm, 0x30, tbrStart);
CHECK_OFFSET(OSAlarm, 0x38, userData);
CHECK_OFFSET(OSAlarm, 0x3c, state);
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

void
OSInitAlarmQueue(OSAlarmQueue *queue);

void
OSInitAlarmQueueEx(OSAlarmQueue *queue, const char *name);

BOOL
OSSetAlarm(OSAlarm *alarm, OSTime time, AlarmCallback callback);

BOOL
OSSetPeriodicAlarm(OSAlarm *alarm, OSTime start, OSTime interval, AlarmCallback callback);

void
OSSetAlarmTag(OSAlarm *alarm, uint32_t alarmTag);

void
OSSetAlarmUserData(OSAlarm *alarm, void *data);

BOOL
OSWaitAlarm(OSAlarm *alarm);

/** @} */

namespace internal
{

void
startAlarmCallbackThreads();

BOOL
setAlarmInternal(OSAlarm *alarm, OSTime time, AlarmCallback callback, void *userData);

bool
cancelAlarm(OSAlarm *alarm);

void
handleAlarmInterrupt(OSContext *context);

} // namespace internal

} // namespace coreinit
