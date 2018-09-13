#pragma once
#include "ios_mcp_enum.h"
#include "ios_mcp_mcp_types.h"

namespace ios::mcp::internal
{

phys_ptr<MCPPPrepareTitleInfo>
getPrepareTitleInfoBuffer();

MCPError
readTitleCosXml(phys_ptr<MCPPPrepareTitleInfo> titleInfo);

void
initialiseTitleStaticData();

} // namespace ios::mcp::internal
