#pragma once
#include "coreinit_enum.h"
#include "coreinit_ios.h"
#include "ios/dev/mcp.h"

#include <common/be_val.h>
#include <common/structsize.h>
#include <cstdint>

namespace coreinit
{

/**
 * \defgroup coreinit_mcp MCP
 * \ingroup coreinit
 * @{
 */

using ios::dev::mcp::MCPAppType;
using ios::dev::mcp::MCPCountryCode;
using ios::dev::mcp::MCPError;
using ios::dev::mcp::MCPRegion;
using ios::dev::mcp::MCPSysProdSettings;
using ios::dev::mcp::MCPTitleListType;
using ios::dev::mcp::MCPTitleListSearchFlags;

IOSError
MCP_Open();

void
MCP_Close(IOSHandle handle);

MCPError
MCP_GetOwnTitleInfo(IOSHandle handle,
                    MCPTitleListType *titleInfo);

MCPError
MCP_GetSysProdSettings(IOSHandle handle,
                       MCPSysProdSettings *settings);

MCPError
MCP_GetTitleId(IOSHandle handle,
               be_val<uint64_t> *titleId);

MCPError
MCP_GetTitleInfo(IOSHandle handle,
                 uint64_t titleId,
                 MCPTitleListType *titleInfo);

MCPError
MCP_TitleCount(IOSHandle handle);

MCPError
MCP_TitleList(IOSHandle handle,
              uint32_t *titleCount,
              MCPTitleListType *titleList,
              uint32_t titleListSizeBytes);

MCPError
MCP_TitleListByAppType(IOSHandle handle,
                       MCPAppType appType,
                       uint32_t *titleCount,
                       MCPTitleListType *titleList,
                       uint32_t titleListSizeBytes);

MCPError
MCP_TitleListByUniqueId(IOSHandle handle,
                        uint32_t uniqueId,
                        uint32_t *titleCount,
                        MCPTitleListType *titleList,
                        uint32_t titleListSizeBytes);

MCPError
MCP_TitleListByUniqueIdAndIndexedDeviceAndAppType(IOSHandle handle,
                                                  uint32_t uniqueId,
                                                  const char *indexedDevice,
                                                  uint8_t unk0x60,
                                                  MCPAppType appType,
                                                  uint32_t *titleCount,
                                                  MCPTitleListType *titleList,
                                                  uint32_t titleListSizeBytes);

namespace internal
{

void
mcpInit();

void *
mcpAllocateMessage(uint32_t size);

MCPError
mcpFreeMessage(void *message);

MCPError
mcpDecodeIosErrorToMcpError(IOSError error);

IOSError
mcpSearchTitleList(IOSHandle handle,
                   MCPTitleListType *searchTitle,
                   MCPTitleListSearchFlags searchFlags,
                   MCPTitleListType *titleListOut,
                   uint32_t titleListLength);

} // namespace internal

/** @} */

} // namespace coreinit
