#include "nn_save.h"
#include "nn_save_core.h"

namespace nn
{

namespace save
{

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
Module::registerCoreFunctions()
{
   RegisterKernelFunction(SAVEInit);
   RegisterKernelFunction(SAVEShutdown);
}

} // namespace save

} // namespace nn
