#include <hle_test.h>
#include <coreinit/messagequeue.h>

#define NumMessages 1

OSMessageQueue sQueue;
OSMessage sMessages[NumMessages];

int main(int argc, char **argv)
{
   OSMessage sendMsg, peekMsg, recvMsg;
   OSInitMessageQueue(&sQueue, sMessages, NumMessages);

   // Send message.
   sendMsg.message = (void *)1;
   sendMsg.args[0] = 2;
   sendMsg.args[1] = 3;
   sendMsg.args[2] = 4;
   test_eq(OSSendMessage(&sQueue, &sendMsg, 0), TRUE);

   // Peek message, which should not remove it from queue.
   test_eq(OSPeekMessage(&sQueue, &peekMsg), TRUE);
   test_eq(peekMsg.message, sendMsg.message);
   test_eq(peekMsg.args[0], sendMsg.args[0]);
   test_eq(peekMsg.args[1], sendMsg.args[1]);
   test_eq(peekMsg.args[2], sendMsg.args[2]);

   // Ensure that we can still receive the message.
   test_eq(OSReceiveMessage(&sQueue, &recvMsg, 0), TRUE);
   test_eq(recvMsg.message, sendMsg.message);
   test_eq(recvMsg.args[0], sendMsg.args[0]);
   test_eq(recvMsg.args[1], sendMsg.args[1]);
   test_eq(recvMsg.args[2], sendMsg.args[2]);

   return 0;
}
