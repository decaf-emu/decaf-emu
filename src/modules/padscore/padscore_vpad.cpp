#include "padscore.h"
#include "padscore_vpad.h"

void
VPADInit()
{
}

void
PadScore::registerVPADFunctions()
{
   RegisterKernelFunction(VPADInit);
}
