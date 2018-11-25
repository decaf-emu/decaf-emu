#include <hle_test.h>
#include <coreinit/alarm.h>
#include <coreinit/systeminfo.h>

OSAlarm sPeriodicAlarm;

#define NUM_ALARM_FIRED 5
int sAlarmFiredCount = 0;

void
AlarmCallback(OSAlarm *alarm, OSContext *context)
{
   test_assert(alarm == &sPeriodicAlarm);
   test_report("AlarmCallback called.");
   sAlarmFiredCount++;

   if (sAlarmFiredCount == NUM_ALARM_FIRED) {
      OSCancelAlarm(alarm);
   }
}

int main(int argc, char **argv)
{
   // Start a periodic alarm
   OSCreateAlarmEx(&sPeriodicAlarm, "PeriodicAlarm");
   OSSetPeriodicAlarm(&sPeriodicAlarm,
                      OSGetTime() + OSMillisecondsToTicks(10),
                      OSMillisecondsToTicks(10),
                      &AlarmCallback);

   // Wait for 100ms to allow periodic alarm to fire.
   OSAlarm alarm;
   OSCreateAlarmEx(&alarm, "TestAlarm");
   OSSetAlarm(&alarm, OSMillisecondsToTicks(100), NULL);
   OSWaitAlarm(&alarm);

   test_report("Alarm fired count: %d", sAlarmFiredCount);
   test_assert(sAlarmFiredCount == NUM_ALARM_FIRED);
   return 0;
}
