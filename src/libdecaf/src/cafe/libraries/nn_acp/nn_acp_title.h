#pragma once
#include "cafe/libraries/nn_result.h"

#include <libcpu/be2_struct.h>

namespace cafe::nn::acp
{

struct ACPMetaXml
{
   UNKNOWN(0x3440);
};
CHECK_SIZE(ACPMetaXml, 0x3440);

nn::Result
GetTitleMetaXml(uint64_t titleId,
                virt_ptr<ACPMetaXml> data);

}  // namespace cafe::nn::acp
