#include <hle_test.h>
#include <coreinit/messagequeue.h>

#define NumMessages 16

OSMessageQueue sQueue;
OSMessage sMessages[NumMessages];

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
   test_eq(OSSendMessage(&sQueue, &msg, OS_MESSAGE_FLAGS_NONE), FALSE);

   return 0;
}
