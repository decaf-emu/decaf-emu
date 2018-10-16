#include "nn_acp.h"
#include "nn_acp_title.h"

#include "cafe/libraries/cafe_hle_stub.h"
#include "nn/acp/nn_acp_result.h"

using namespace nn::acp;

namespace cafe::nn_acp
{

nn::Result
GetTitleMetaXml(uint64_t titleId,
                virt_ptr<ACPMetaXml> data)
{
   decaf_warn_stub();
   return ResultSuccess;
}

void
Library::registerTitleSymbols()
{
   RegisterFunctionExportName("ACPGetTitleMetaXml", GetTitleMetaXml);
   RegisterFunctionExportName("GetTitleMetaXml__Q2_2nn3acpFULP11_ACPMetaXml",
                              GetTitleMetaXml);
}

}  // namespace cafe::nn_acp
