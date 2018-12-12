#pragma once
#include "nn/acp/nn_acp_types.h"
#include "nn/nn_result.h"

namespace ios::acp::internal
{

nn::Result
loadMetaXMLFromPath(std::string_view path,
                    phys_ptr<nn::acp::ACPMetaXml> metaXml);

} // namespace ios::acp::internal
