#pragma once
#include "nn/nn_result.h"
#include "nn/acp/nn_acp_miscservice.h"

#include <libcpu/be2_struct.h>

namespace cafe::nn_acp
{

nn::Result
ACPGetNetworkTime(virt_ptr<int64_t> outTime,
                  virt_ptr<uint32_t> outUnknown);

nn::Result
ACPGetTitleMetaXml(uint64_t titleId,
                   virt_ptr<nn::acp::ACPMetaXml> outData);

}  // namespace cafe::nn_acp
