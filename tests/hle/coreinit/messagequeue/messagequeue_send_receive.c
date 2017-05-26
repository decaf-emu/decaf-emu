#include <hle_test.h>
#include <coreinit/messagequeue.h>

#define NumMessages 2

OSMessageQueue sQueue;
OSMessage sMessages[NumMessages];

int main(int argc, char **argv)
{
   OSMessage sendMsg1, sendMsg2, recvMsg;
   OSInitMessageQueue(&sQueue, sMessages, NumMessages);

   // Send first message.
   sendMsg1.message = (void *)1;
   sendMsg1.args[0] = 2;
   sendMsg1.args[1] = 3;
   sendMsg1.args[2] = 4;
   test_eq(OSSendMessage(&sQueue, &sendMsg1, 0), TRUE);

   // Send second message..
   sendMsg2.message = (void *)2;
   sendMsg2.args[0] = 3;
   sendMsg2.args[1] = 4;
   sendMsg2.args[2] = 5;
   test_eq(OSSendMessage(&sQueue, &sendMsg2, 0), TRUE);

   // Receive first message.
   test_eq(OSReceiveMessage(&sQueue, &recvMsg, 0), TRUE);
   test_eq(recvMsg.message, sendMsg1.message);
   test_eq(recvMsg.args[0], sendMsg1.args[0]);
   test_eq(recvMsg.args[1], sendMsg1.args[1]);
   test_eq(recvMsg.args[2], sendMsg1.args[2]);

   // Receive second message.
   test_eq(OSReceiveMessage(&sQueue, &recvMsg, 0), TRUE);
   test_eq(recvMsg.message, sendMsg2.message);
   test_eq(recvMsg.args[0], sendMsg2.args[0]);
   test_eq(recvMsg.args[1], sendMsg2.args[1]);
   test_eq(recvMsg.args[2], sendMsg2.args[2]);

   // The queue should now be empty.
   test_eq(OSReceiveMessage(&sQueue, &recvMsg, 0), FALSE);

   return 0;
}
