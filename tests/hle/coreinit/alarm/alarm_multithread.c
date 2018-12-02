#include <hle_test.h>
#include <coreinit/alarm.h>
#include <coreinit/core.h>
#include <coreinit/debug.h>
#include <coreinit/thread.h>
#include <coreinit/time.h>
#include <coreinit/systeminfo.h>

#define STACK_SIZE 4096

OSThread threadCore0 = { 0 };
OSThread threadCore2 = { 0 };
uint8_t stackCore0[STACK_SIZE] = { 0 };
uint8_t stackCore2[STACK_SIZE] = { 0 };

int periodicFireCount = 0;
BOOL core0Fired = FALSE;
BOOL core2Fired = FALSE;

void
PeriodicAlarmHandler(OSAlarm *alarm, OSContext *context)
{
   test_report("PeriodicAlarm fired");
   periodicFireCount++;
}

void
Core0AlarmHandler(OSAlarm *alarm, OSContext *context)
{
   test_report("Core0_Alarm fired");
   core0Fired = TRUE;
}

void
Core2AlarmHandler(OSAlarm *alarm, OSContext *context)
{
   test_report("Core2_Alarm fired");
   core2Fired = TRUE;
}

int
CoreEntryPoint0(int argc, const char **argv)
{
   OSAlarm alarm;
   OSCreateAlarmEx(&alarm, "Core0_Alarm");
   OSSetAlarm(&alarm, OSMillisecondsToTicks(225), &Core0AlarmHandler);
   OSWaitAlarm(&alarm);

   test_assert(core0Fired);
   test_report("Core0 exit");
   return core0Fired ? 1 : 0;
}

int
CoreEntryPoint2(int argc, const char **argv)
{
   OSAlarm alarm;
   OSCreateAlarmEx(&alarm, "Core2_Alarm");
   OSSetAlarm(&alarm, OSMillisecondsToTicks(125), &Core2AlarmHandler);
   OSSleepTicks(OSMillisecondsToTicks(175));

   test_assert(core2Fired);
   test_report("Core2 finished");
   return core2Fired ? 1 : 0;
}

int
main(int argc, char **argv)
{
   OSAlarm periodicAlarm;
   OSCreateAlarmEx(&periodicAlarm, "PeriodicAlarm");
   OSSetPeriodicAlarm(&periodicAlarm,
                      OSGetTime() + OSMillisecondsToTicks(50),
                      OSMillisecondsToTicks(50),
                      &PeriodicAlarmHandler);

   test_assert(OSGetCoreId() == 1);

   // Run thread on core 0
   OSCreateThread(&threadCore0, CoreEntryPoint0, 0, NULL, stackCore0 + STACK_SIZE, STACK_SIZE, 20, OS_THREAD_ATTRIB_AFFINITY_CPU0);
   OSResumeThread(&threadCore0);

   // Run thread on core 2
   OSCreateThread(&threadCore2, CoreEntryPoint2, 0, NULL, stackCore2 + STACK_SIZE, STACK_SIZE, 20, OS_THREAD_ATTRIB_AFFINITY_CPU2);
   OSResumeThread(&threadCore2);

   // Wait for threads to return
   int resultCore0 = -1, resultCore2 = -1;
   OSJoinThread(&threadCore0, &resultCore0);
   OSJoinThread(&threadCore2, &resultCore2);

   OSCancelAlarm(&periodicAlarm);

   test_report("Core 1 fire count %d", periodicFireCount);
   test_report("Core 0 thread returned %d", resultCore0);
   test_report("Core 2 thread returned %d", resultCore2);
   return 0;
}
