#pragma once
#include "types.h"
#include "modules/nn_result.h"
#include "utils/structsize.h"

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
GetTitleMetaXml(uint64_t id, ACPMetaXml *data);

}  // namespace acp

}  // namespace nn
