#pragma once
#include "modules/nn_result.h"

#include <common/structsize.h>

#include <cstdint>

namespace nn
{

namespace acp
{

struct ACPMetaXml
{
   UNKNOWN(0x3440);
};
CHECK_SIZE(ACPMetaXml, 0x3440);

nn::Result
GetTitleMetaXml(uint64_t id,
                ACPMetaXml *data);

}  // namespace acp

}  // namespace nn
