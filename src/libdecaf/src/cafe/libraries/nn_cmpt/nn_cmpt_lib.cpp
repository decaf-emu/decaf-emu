#include "nn_cmpt.h"
#include "nn_cmpt_lib.h"

namespace cafe::nn::cmpt
{

CMPTError
CMPTGetDataSize(virt_ptr<uint32_t> outDataSize)
{
   *outDataSize = 12 * 1024 * 1024;
   return CMPTError::OK;
}

void
Library::registerLibSymbols()
{
   RegisterFunctionExport(CMPTGetDataSize);
}

} // namespace cafe::nn::cmpt
