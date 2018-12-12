#pragma once
#include "nn/nn_result.h"
#include "nn/acp/nn_acp_miscservice.h"

#include <libcpu/be2_struct.h>

namespace cafe::nn_acp
{

using ACPMetaXml = nn::acp::ACPMetaXml;
using ACPTitleId = nn::acp::ACPTitleId;

nn::Result
ACPGetNetworkTime(virt_ptr<int64_t> outTime,
                  virt_ptr<uint32_t> outUnknown);

nn::Result
ACPGetTitleIdOfMainApplication(virt_ptr<ACPTitleId> outTitleId);

nn::Result
ACPGetTitleMetaXml(ACPTitleId titleId,
                   virt_ptr<ACPMetaXml> outData);

}  // namespace cafe::nn_acp
