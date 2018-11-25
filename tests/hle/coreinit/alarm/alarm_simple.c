#include <hle_test.h>
#include <coreinit/alarm.h>
#include <coreinit/systeminfo.h>

OSAlarm sAlarm;

void
AlarmCallback(OSAlarm *alarm, OSContext *context)
{
   test_assert(alarm == &sAlarm);
   test_report("AlarmCallback called.");
}

int main(int argc, char **argv)
{
   OSCreateAlarm(&sAlarm);

   // Test with valid alarm callback
   OSSetAlarm(&sAlarm, OSMillisecondsToTicks(50), &AlarmCallback);
   OSWaitAlarm(&sAlarm);

   // Test with NULL alarm callback
   OSSetAlarm(&sAlarm, OSMillisecondsToTicks(50), NULL);
   OSWaitAlarm(&sAlarm);
   return 0;
}
