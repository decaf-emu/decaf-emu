#include "nn_temp.h"
#include "nn_temp_core.h"

namespace nn
{

namespace temp
{

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
Module::registerCoreFunctions()
{
   RegisterKernelFunction(TEMPInit);
   RegisterKernelFunction(TEMPShutdown);
}

} // namespace temp

} // namespace nn
