#include "vpad.h"
#include "vpad_status.h"

int32_t
VPADRead(uint32_t chan, VPADStatus *buffers, uint32_t count, be_val<int32_t> *error)
{
   assert(count >= 1);
   memset(&buffers[0], 0, sizeof(VPADStatus));

   if (error) {
      *error = 0;
   }

   return 1;
}

void
VPad::registerStatusFunctions()
{
   RegisterKernelFunction(VPADRead);
}
