#include "nn_cmpt.h"
#include "nn_cmpt_lib.h"

namespace cafe::nn_cmpt
{

CMPTError
CMPTGetDataSize(virt_ptr<uint32_t> outDataSize)
{
   *outDataSize = 12u * 1024u * 1024u;
   return CMPTError::OK;
}

void
Library::registerLibSymbols()
{
   RegisterFunctionExport(CMPTGetDataSize);
}

} // namespace cafe::nn_cmpt
