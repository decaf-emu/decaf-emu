#include <hle_test.h>
#include <coreinit/alarm.h>
#include <coreinit/systeminfo.h>

OSAlarm sAlarm;

void
AlarmCallback(OSAlarm *alarm, OSContext *context)
{
   test_fail("AlarmCallback should not have been called.");
}

int main(int argc, char **argv)
{
   OSCreateAlarmEx(&sAlarm, "Simple_Alarm");
   OSSetAlarm(&sAlarm, OSMilliseconds(50), &AlarmCallback);

   // Set alarm group and cancel the group
   OSSetAlarmTag(&sAlarm, 1337);
   OSCancelAlarms(1337);

   // Sleep until after alarm was due to go off to test cancellation.
   OSSleepTicks(OSMilliseconds(100));
   return 0;
}
