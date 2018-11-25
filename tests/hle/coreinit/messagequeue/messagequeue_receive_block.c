#include <hle_test.h>
#include <coreinit/messagequeue.h>
#include <coreinit/systeminfo.h>
#include <coreinit/thread.h>
#include <coreinit/time.h>

#define NumMessages 16
#define StackSize 4096

OSMessageQueue sQueue;
OSMessage sMessages[NumMessages];
int sMessagesRead;

OSThread sThread;
uint8_t sThreadStack[StackSize];

int msgThreadEntry(int argc, const char **argv)
{
   // Give main thread a chance to block on the message queue
   OSMessage msg;
   sMessagesRead = 0;

   while (sMessagesRead < NumMessages) {
      // Receive message
      test_eq(OSReceiveMessage(&sQueue, &msg, OS_MESSAGE_FLAGS_BLOCKING), TRUE);
      test_eq(msg.message, (void *)1);
      test_eq(msg.args[0], 2 + sMessagesRead);
      test_eq(msg.args[1], 3);
      test_eq(msg.args[2], 4);
      ++sMessagesRead;
   }

   return 0;
}

int main(int argc, char **argv)
{
   OSMessage msg;
   int i;

   OSInitMessageQueue(&sQueue, sMessages, NumMessages);

   OSCreateThread(&sThread, msgThreadEntry, 0, NULL, sThreadStack + StackSize, StackSize, 20, 0);
   OSResumeThread(&sThread);

   // Make sure other thread gets to a blocking read
   OSSleepTicks(OSMillisecondsToTicks(10));

   // Sending message into empty queue should succeed.
   for (i = 0; i < NumMessages; ++i) {
      msg.message = (void *)1;
      msg.args[0] = 2 + i;
      msg.args[1] = 3;
      msg.args[2] = 4;
      test_eq(OSSendMessage(&sQueue, &msg, 0), TRUE);
   }

   // Make sure other thread gets a chance to read all messages.
   OSSleepTicks(OSMillisecondsToTicks(10));
   test_eq(sMessagesRead, NumMessages);

   return 0;
}
