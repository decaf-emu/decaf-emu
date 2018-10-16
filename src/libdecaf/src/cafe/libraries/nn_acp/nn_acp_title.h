#pragma once
#include "nn/nn_result.h"

#include <libcpu/be2_struct.h>

namespace cafe::nn_acp
{

struct ACPMetaXml
{
   UNKNOWN(0x3440);
};
CHECK_SIZE(ACPMetaXml, 0x3440);

nn::Result
GetTitleMetaXml(uint64_t titleId,
                virt_ptr<ACPMetaXml> data);

}  // namespace cafe::nn_acp
