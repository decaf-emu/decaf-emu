#include <hle_test.h>
#include <coreinit/coroutine.h>

#define StackSize 8192
static uint8_t coStack1[StackSize] __attribute__((aligned(8)));
static uint8_t coStack2[StackSize] __attribute__((aligned(8)));

OSCoroutine coMain;
OSCoroutine coOther1;
OSCoroutine coOther2;
int coValue1 = 0;
int coValue2 = 0;
int coValueBoth = 0;

void coEntryPoint1()
{
   test_report("coEntryPoint1");

   while (TRUE) {
      coValue1++;
      coValueBoth++;
      OSSwitchCoroutine(&coOther1, &coOther2);
   }
}

void coEntryPoint2()
{
   test_report("coEntryPoint2");

   while (TRUE) {
      coValue2++;
      coValueBoth++;
      OSSwitchCoroutine(&coOther2, &coMain);
   }
}

int main(int argc, char **argv)
{
   uint8_t *coStackPtr1 = coStack1 + StackSize - 8;
   OSInitCoroutine(&coOther1, coEntryPoint1, coStackPtr1);

   uint8_t *coStackPtr2 = coStack2 + StackSize - 8;
   OSInitCoroutine(&coOther2, coEntryPoint2, coStackPtr2);

   test_report("main");

   for (int i = 0; i < 5; ++i) {
      OSSwitchCoroutine(&coMain, &coOther1);
   }

   test_report("Returned to coMain");
   test_assert(coValue1 == 5);
   test_assert(coValue2 == 5);
   test_assert(coValueBoth == 10);
   return 0;
}
