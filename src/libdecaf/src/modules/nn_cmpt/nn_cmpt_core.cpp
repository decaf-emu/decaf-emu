#include "nn_cmpt.h"
#include "nn_cmpt_core.h"

namespace nn
{

namespace cmpt
{

CMPTError
CMPTGetDataSize(be_val<uint32_t> *size)
{
   *size = 12 * 1024 * 1024; // 12 MB
   return 0;
}

void
Module::registerCoreFunctions()
{
   RegisterKernelFunction(CMPTGetDataSize);
}

} // namespace cmpt

} // namespace nn
