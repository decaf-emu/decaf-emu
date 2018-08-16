#pragma once
#include "coreinit_context.h"
#include "coreinit_enum.h"
#include "coreinit_thread.h"
#include "coreinit_time.h"

#include <libcpu/be2_struct.h>

namespace cafe::coreinit
{

/**
 * \defgroup coreinit_alarms Alarms
 * \ingroup coreinit
 * @{
 */

#pragma pack(push, 1)

struct OSAlarm;
using AlarmCallbackFn = virt_func_ptr<void(virt_ptr<OSAlarm> alarm,
                                           virt_ptr<OSContext> context)>;

struct OSAlarmQueue
{
   static constexpr uint32_t Tag = 0x614C6D51;

   //! Should always be set to the value OSAlarmQueue::Tag.
   be2_val<uint32_t> tag;

   //! Name set from OSInitAlarmQueueEx.
   be2_virt_ptr<const char> name;

   UNKNOWN(4);

   //! List of threads waiting on this alarm queue.
   be2_struct<OSThreadQueue> threadQueue;

   //! First alarm in the queue.
   be2_virt_ptr<OSAlarm> head;

   //! Last alarm in the queue.
   be2_virt_ptr<OSAlarm> tail;
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
   be2_virt_ptr<OSAlarm> prev;

   //! Next alarm in the queue.
   be2_virt_ptr<OSAlarm> next;
};
CHECK_OFFSET(OSAlarmLink, 0x00, prev);
CHECK_OFFSET(OSAlarmLink, 0x04, next);
CHECK_SIZE(OSAlarmLink, 0x08);

struct OSAlarm
{
   static const uint32_t Tag = 0x614C724D;

   //! Should always be set to the value OSAlarm::Tag.
   be2_val<uint32_t> tag;

   //! Name set from OSCreateAlarmEx.
   be2_virt_ptr<const char> name;

   UNKNOWN(4);

   //! The callback to execute once the alarm is triggered.
   be2_val<AlarmCallbackFn> callback;

   //! Used with OSCancelAlarms for bulk cancellation of alarms.
   be2_val<uint32_t> group;

   UNKNOWN(4);

   //! The time when the alarm will next be triggered.
   be2_val<OSTime> nextFire;

   //! Link used for when this OSAlarm object is inside an OSAlarmQueue
   be2_struct<OSAlarmLink> link;

   //! The period between alarm triggers, this is only set for periodic alarms.
   be2_val<OSTime> period;

   //! The time the alarm was started.
   be2_val<OSTime> tbrStart;

   //! User data set with OSSetAlarmUserData and retrieved with OSGetAlarmUserData.
   be2_virt_ptr<void> userData;

   //! The current state of the alarm.
   be2_val<OSAlarmState> state;

   //! Queue of threads currently waiting for the alarm to trigger with OSWaitAlarm.
   be2_struct<OSThreadQueue> threadQueue;

   //! The queue that this alarm is currently in.
   be2_virt_ptr<OSAlarmQueue> alarmQueue;

   //! The context the alarm was triggered on.
   be2_virt_ptr<OSContext> context;
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
OSCancelAlarm(virt_ptr<OSAlarm> alarm);

void
OSCancelAlarms(uint32_t alarmTag);

void
OSCreateAlarm(virt_ptr<OSAlarm> alarm);

void
OSCreateAlarmEx(virt_ptr<OSAlarm> alarm,
                virt_ptr<const char> name);

virt_ptr<void>
OSGetAlarmUserData(virt_ptr<OSAlarm> alarm);

void
OSInitAlarmQueue(virt_ptr<OSAlarmQueue> queue);

void
OSInitAlarmQueueEx(virt_ptr<OSAlarmQueue> queue,
                   virt_ptr<const char> name);

BOOL
OSSetAlarm(virt_ptr<OSAlarm> alarm,
           OSTime time,
           AlarmCallbackFn callback);

BOOL
OSSetPeriodicAlarm(virt_ptr<OSAlarm> alarm,
                   OSTime start,
                   OSTime interval,
                   AlarmCallbackFn callback);

void
OSSetAlarmTag(virt_ptr<OSAlarm> alarm,
              uint32_t alarmTag);

void
OSSetAlarmUserData(virt_ptr<OSAlarm> alarm,
                   virt_ptr<void> data);

BOOL
OSWaitAlarm(virt_ptr<OSAlarm> alarm);

namespace internal
{

BOOL
setAlarmInternal(virt_ptr<OSAlarm> alarm,
                 OSTime time,
                 AlarmCallbackFn callback,
                 virt_ptr<void> userData);

bool
cancelAlarm(virt_ptr<OSAlarm> alarm);

void
handleAlarmInterrupt(virt_ptr<OSContext> context);

void
initialiseAlarmThread();

} // namespace internal

/** @} */

} // namespace cafe::coreinit
