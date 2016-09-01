#include "nn_acp.h"
#include "nn_acp_title.h"

namespace nn
{

namespace acp
{

nn::Result
GetTitleMetaXml(uint64_t id, ACPMetaXml *data)
{
   decaf_warn_stub();

   return nn::Result::Success;
}

void
Module::registerTitleFunctions()
{
   RegisterKernelFunctionName("ACPGetTitleMetaXml", nn::acp::GetTitleMetaXml);
   RegisterKernelFunctionName("GetTitleMetaXml__Q2_2nn3acpFULP11_ACPMetaXml", nn::acp::GetTitleMetaXml);
}

}  // namespace acp

}  // namespace nn
