#include <hle_test.h>
#include <coreinit/alarm.h>
#include <coreinit/systeminfo.h>

#define USER_DATA_VALUE 1337
OSAlarm sAlarm;

void
AlarmCallback(OSAlarm *alarm, OSContext *context)
{
   test_assert(alarm == &sAlarm);
   test_report("AlarmCallback called.");

   void *userData = OSGetAlarmUserData(alarm);
   test_assert(userData);
   test_assert((*(int*)userData) == USER_DATA_VALUE);
}

int main(int argc, char **argv)
{
   int testData = USER_DATA_VALUE;
   OSCreateAlarm(&sAlarm);
   OSSetAlarmUserData(&sAlarm, &testData);
   OSSetAlarm(&sAlarm, OSMillisecondsToTicks(50), &AlarmCallback);
   OSWaitAlarm(&sAlarm);
   return 0;
}
