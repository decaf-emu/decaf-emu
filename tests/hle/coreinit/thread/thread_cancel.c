#include <hle_test.h>
#include <coreinit/systeminfo.h>
#include <coreinit/thread.h>
#include <coreinit/time.h>

OSThread gThread;
uint8_t gThreadStack[4096];

int cancelThreadEntry(int argc, const char **argv)
{
   OSTime lastCheck = OSGetTime();

   while (1) {
      OSTime now = OSGetTime();

      if (now - lastCheck >= OSMillisecondsToTicks(1)) {
         OSTestThreadCancel();
         test_report("Cancel thread still running");
         lastCheck = now;
      }
   }

   return 0;
}

int main(int argc, char **argv)
{
   test_report("Setup");
   OSCreateThread(&gThread, cancelThreadEntry, 0, NULL, gThreadStack + 4096, 4096, 20, 0);

   test_report("Start the thread");
   OSResumeThread(&gThread);

   test_report("Main thread sleeping for 5 milliseconds");
   OSSleepTicks(OSMillisecondsToTicks(5));

   test_report("Try to cancel thread");
   OSCancelThread(&gThread);
   OSJoinThread(&gThread, NULL);

   test_report("Cancel thread succeeded");
   return 0;
}
