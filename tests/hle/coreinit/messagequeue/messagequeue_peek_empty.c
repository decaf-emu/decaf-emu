#include <hle_test.h>
#include <coreinit/messagequeue.h>

#define NumMessages 1

OSMessageQueue sQueue;
OSMessage sMessages[NumMessages];

int main(int argc, char **argv)
{
   OSMessage sendMsg, peekMsg, recvMsg;
   OSInitMessageQueue(&sQueue, sMessages, NumMessages);

   // Peek message on empty queue should return FALSE.
   test_eq(OSPeekMessage(&sQueue, &peekMsg), FALSE);

   return 0;
}
