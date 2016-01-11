#include "nn_save.h"
#include "nn_save_core.h"

SaveStatus
SAVEInit()
{
   return SaveStatus::OK;
}

void
SAVEShutdown()
{
}

void
NN_save::registerCoreFunctions()
{
   RegisterKernelFunction(SAVEInit);
   RegisterKernelFunction(SAVEShutdown);
}
