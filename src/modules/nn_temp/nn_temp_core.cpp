#include "nn_temp.h"
#include "nn_temp_core.h"

TempStatus
TEMPInit()
{
   return TempStatus::OK;
}

void
TEMPShutdown()
{
}

void
NN_temp::registerCoreFunctions()
{
   RegisterKernelFunction(TEMPInit);
   RegisterKernelFunction(TEMPShutdown);
}
