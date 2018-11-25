#include <hle_test.h>
#include <coreinit/messagequeue.h>
#include <coreinit/systeminfo.h>
#include <coreinit/thread.h>
#include <coreinit/time.h>

#define NumMessages 16

OSMessageQueue sQueue;
OSMessage sMessages[NumMessages];

OSThread sThread;
uint8_t sThreadStack[4096];

int msgThreadEntry(int argc, const char **argv)
{
   // Give main thread a chance to block on the message queue
   OSMessage msg;
   OSSleepTicks(OSMillisecondsToTicks(10));

   // Receive message
   test_eq(OSReceiveMessage(&sQueue, &msg, 0), TRUE);
   test_eq(msg.message, (void *)1);
   test_eq(msg.args[0], 2);
   test_eq(msg.args[1], 3);
   test_eq(msg.args[2], 4);

   return 0;
}

int main(int argc, char **argv)
{
   OSMessage msg;
   int i;

   OSInitMessageQueue(&sQueue, sMessages, NumMessages);

   // Sending message into empty queue should succeed.
   for (i = 0; i < NumMessages; ++i) {
      msg.message = (void *)1;
      msg.args[0] = 2 + i;
      msg.args[1] = 3;
      msg.args[2] = 4;
      test_eq(OSSendMessage(&sQueue, &msg, 0), TRUE);
   }

   // Sending message into full queue should fail.
   test_eq(OSSendMessage(&sQueue, &msg, 0), FALSE);

   // Sending message into full queue with block flag should return success
   // once another thread has read a message from the queue to free up space.
   OSCreateThread(&sThread, msgThreadEntry, 0, NULL, sThreadStack + 4096, 4096, 20, 0);
   OSResumeThread(&sThread);
   test_eq(OSSendMessage(&sQueue, &msg, OS_MESSAGE_FLAGS_BLOCKING), TRUE);

   return 0;
}
