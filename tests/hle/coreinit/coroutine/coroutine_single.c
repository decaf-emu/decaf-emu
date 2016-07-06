#include <hle_test.h>
#include <coreinit/coroutine.h>

#define StackSize 8192
static uint8_t coStack[StackSize] __attribute__((aligned(8)));

OSCoroutine coMain;
OSCoroutine coOther;
int coValue = 0;

void coEntryPoint()
{
   test_report("coEntryPoint");
   coValue = 1;

   test_report("Switching from coOther to coMain");
   OSSwitchCoroutine(&coOther, &coMain);
}

int main(int argc, char **argv)
{
   uint8_t *coStackPtr = coStack + StackSize - 8;
   OSInitCoroutine(&coOther, coEntryPoint, coStackPtr);

   test_report("Switching from coMain to coOther");
   OSSwitchCoroutine(&coMain, &coOther);

   test_report("Returned to coMain");
   test_assert(coValue == 1);
   return 0;
}
